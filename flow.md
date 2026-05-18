# TopSearch — Application Flow

A detailed walkthrough of how TopSearch works internally, from startup to result rendering.

---

## Table of Contents

1. [Startup](#1-startup)
2. [Main Loop](#2-main-loop)
3. [Platform Selection](#3-platform-selection)
4. [GitHub Flow](#4-github-flow)
   - [Profiles](#41-profiles)
   - [Repositories](#42-repositories)
   - [Skills](#43-skills)
5. [GitLab Flow](#5-gitlab-flow)
6. [npm Flow](#6-npm-flow)
7. [Stack Overflow Flow](#7-stack-overflow-flow)
8. [HTTP Layer](#8-http-layer)
   - [Cache Layer](#8a-cache-layer)
9. [Query Normalization](#9-query-normalization)
10. [Result Rendering](#10-result-rendering)
11. [Navigation & Back](#11-navigation--back)
12. [Component Diagram](#12-component-diagram)
13. [Data Flow Diagram](#13-data-flow-diagram)

---

## 1. Startup

```
main()
  │
  ├── enableANSI()         — enables ANSI escape codes on Windows console
  ├── SetConsoleOutputCP() — sets output to UTF-8
  ├── SearchEngine engine  — instantiates the search dispatcher
  └── enters main loop
```

- ANSI color codes (`\033[...m`) are activated via `SetConsoleMode` with `ENABLE_VIRTUAL_TERMINAL_PROCESSING`.
- UTF-8 output is enabled so special characters render correctly.
- A single `SearchEngine` instance is reused across all searches (its result vectors are cleared on each new search).
- The GitHub token is read from the `GITHUB_TOKEN` environment variable at startup (not hardcoded).

---

## 2. Main Loop

```
while (true)
  │
  ├── cls() + printBanner()     — clear screen, show ASCII art header
  ├── pickMenu("Select platform") — show platform options
  │     ├── [1] GitHub
  │     ├── [2] GitLab
  │     ├── [3] npm
  │     ├── [4] Stack Overflow
  │     └── [5] Exit → break
  │
  ├── if GitHub → pickMenu("GitHub search type")
  │     ├── [1] Profiles
  │     ├── [2] Repositories
  │     ├── [3] Skills
  │     └── [0] Back → continue (restart loop)
  │
  ├── cls() + printBanner() + breadcrumb
  ├── prompt query input
  │     └── "0" or empty → continue (restart loop)
  │
  ├── engine.search(query, platform, searchType)
  │
  ├── cls() + printBanner() + statusMessage
  ├── render results (platform-specific)
  └── "Press Enter to continue..." → loop
```

---

## 3. Platform Selection

`pickMenu()` is a reusable menu renderer that:

1. Clears the screen and reprints the banner.
2. Lists all options with color-coded `[N]` indices.
3. Optionally shows `[0] Back`.
4. Validates input — on invalid entry, shows `"Invalid, try again..."` for 900ms, then clears and reprints the menu.
5. Returns a 0-based index, or `-1` for Back.

```
pickMenu(prompt, options, showBack)
  │
  ├── cls() + printBanner() + print options
  ├── read integer input
  ├── valid? → return index - 1
  └── invalid?
        ├── print red error message
        ├── sleep 900ms
        └── cls() + reprint menu
```

---

## 4. GitHub Flow

All GitHub requests go through `GitHubAPI`, which delegates to `HttpClient::get()` with the GitHub REST API base URL. The `Authorization: token <PAT>` header is attached automatically for higher rate limits (5000 req/hr vs 60 unauthenticated).

### 4.1 Profiles

```
User enters query
  │
  └── GitHubAPI::searchUsers(query)
        └── GET https://api.github.com/search/users?q={query}
              │
              └── parse JSON → items[]
                    └── extract login → Profile.username
                          │
                          └── store in engine.profiles[]
                                │
                                └── optional: rankProfiles()
                                      ├── spawn thread per username
                                      ├── GET /users/{username} (parallel)
                                      ├── compute score = followers×3 + repos×2 + following
                                      ├── sort descending by score
                                      └── printProfiles(ranked=true)
```

**Score Formula:**
```
score = (followers × 3) + (public_repos × 2) + following
```
Higher weight on followers reflects community influence. Repos weighted for activity. Following is a minor signal.

---

### 4.2 Repositories

```
User enters query
  │
  └── GitHubAPI::searchRepositories(query)
        └── GET https://api.github.com/search/repositories?q={query}
              │
              └── parse JSON → items[]
                    └── store raw JSON objects in engine.repos[]
                          │
                          └── printReposGitHub()
                                ├── owner/name, URL
                                ├── stars, forks, language
                                └── description

  Optional ZIP download:
    ├── user picks result index
    ├── build URL: https://github.com/{owner}/{name}/archive/refs/heads/{branch}.zip
    └── HttpClient::downloadFile(url, "{name}.zip")
          ├── CURLOPT_FOLLOWLOCATION = true (follows redirects)
          ├── CURLOPT_SSL_VERIFYPEER = true
          ├── CURLOPT_FAILONERROR = true
          └── writes binary to file → saved in working directory
```

---

### 4.3 Skills

Skills search is the most complex flow — it cross-references users and repository owners to find developers with a given skill in their bio.

```
User enters skill query (e.g. "rust")
  │
  ├── Step 1: searchUsers(query)
  │     └── collect login names from items[]
  │
  ├── Step 2: searchRepositories(query)
  │     └── collect owner.login from items[] (deduplicated)
  │
  ├── Merge candidates → max 30 unique usernames
  │
  ├── Step 3: parallel profile fetch (one thread per username)
  │     └── GET /users/{username}
  │           ├── parse name, bio, followers, repos, following
  │           └── check: bio contains skill keyword OR username contains it
  │                 └── match? → push to engine.profiles[] (mutex-protected)
  │
  ├── Sort: bio-match profiles first
  └── printProfiles(ranked=false)
```

**Why parallel?** Fetching up to 30 profiles sequentially would take ~10–30 seconds. Threads reduce this to the time of the slowest single request (~1–2s).

---

## 5. GitLab Flow

```
User enters query
  │
  └── HttpClient::get(GitLab API URL)
        └── GET https://gitlab.com/api/v4/projects
              ?visibility=public
              &search={query}
              &per_page=20
              &order_by=star_count
              &sort=desc
              │
              └── parse JSON array → store in engine.repos[]
                    └── printReposGitLab()
                          ├── name_with_namespace
                          ├── web_url
                          ├── star_count, forks_count
                          └── description
```

Results are pre-sorted by star count descending (most popular first) via the API's `order_by` parameter — no client-side sorting needed.

---

## 6. npm Flow

```
User enters query
  │
  └── HttpClient::get(npm registry URL)
        └── GET https://registry.npmjs.org/-/v1/search
              ?text={query}
              &size=20
              │
              └── parse JSON → objects[]
                    └── each object contains a "package" sub-object
                          └── store in engine.packages[]
                                └── printNpm()
                                      ├── name, version
                                      ├── npmjs.com URL
                                      ├── publisher username
                                      └── description
```

---

## 7. Stack Overflow Flow

```
User enters query
  │
  └── HttpClient::get(StackExchange API URL)
        └── GET https://api.stackexchange.com/2.3/search/advanced
              ?q={query}
              &site=stackoverflow
              &pagesize=20
              &order=desc
              &sort=votes
              &filter=default
              │
              └── parse JSON → items[]
                    └── store in engine.questions[]
                          └── printSO()
                                ├── title, link
                                ├── is_answered → [Answered] / [Open]
                                ├── score, answer_count, view_count
                                └── tags[]
```

Results are sorted by votes descending — highest quality answers surface first.

---

## 8. HTTP Layer

`HttpClient` is a static utility class wrapping libcurl. All network I/O passes through it. Every `get()` call checks `CacheManager` before making a network request.

```
HttpClient::get(url)
  │
  ├── CacheManager::instance().get(url)
  │     ├── hit (not expired) → return cached string immediately
  │     └── miss → continue
  ├── curl_easy_init()
  ├── set headers:
  │     ├── User-Agent: TopSearch
  │     └── Authorization: token {githubToken}  ← only if token set
  ├── CURLOPT_FOLLOWLOCATION = true
  ├── CURLOPT_WRITEFUNCTION = WriteCallback
  │     └── appends chunks to std::string response
  ├── curl_easy_perform()
  ├── curl_easy_cleanup()
  ├── CacheManager::instance().set(url, response)  ← store for 1 hour
  └── return response string

HttpClient::downloadFile(url, filename)
  │
  ├── curl_easy_init()
  ├── fopen(filename, "wb")
  ├── CURLOPT_FOLLOWLOCATION = true
  ├── CURLOPT_SSL_VERIFYPEER = true   ← security: verify SSL cert
  ├── CURLOPT_SSL_VERIFYHOST = 2      ← security: verify hostname
  ├── CURLOPT_FAILONERROR = true      ← fail on HTTP 4xx/5xx
  ├── CURLOPT_WRITEFUNCTION = fwrite  ← write directly to file
  ├── curl_easy_perform()
  ├── curl_easy_cleanup() + fclose()
  └── return (res == CURLE_OK)
```

**Token storage:** `HttpClient::githubToken` is a static string initialized from the `GITHUB_TOKEN` environment variable at startup. It is injected into every GitHub request header automatically.

---

## 8a. Cache Layer

`CacheManager` is a singleton that persists API responses to `cache.json` with a 1-hour TTL. It is thread-safe via a mutex.

```
CacheManager::get(key)
  │
  ├── load cache.json (once, lazy)
  ├── key present?
  │     ├── no  → return ""
  │     └── yes → check timestamp
  │               ├── expired (> 3600s) → erase entry, return ""
  │               └── fresh → return cached value

CacheManager::set(key, value)
  │
  ├── store {ts: now(), val: value} under key
  └── save cache.json
```

This avoids redundant API calls for repeated identical queries within the same hour.

---

## 9. Query Normalization

Before any query is sent to an API, it passes through `HttpClient::normalizeQuery()`:

```cpp
normalizeQuery(query)
  ├── remove all spaces
  └── convert to lowercase
```

Example: `"React Native"` → `"reactnative"`

This ensures consistent API queries regardless of how the user typed the input.

---

## 10. Result Rendering

Each platform has a dedicated print function in `main.cpp`. All use ANSI color codes for visual clarity:

| Function | Platform | Color Scheme |
|---|---|---|
| `printProfiles()` | GitHub users | Blue username, Yellow rank, Green score |
| `printReposGitHub()` | GitHub repos | Blue name, Yellow stars/forks |
| `printReposGitLab()` | GitLab projects | Magenta name, Yellow stars/forks |
| `printNpm()` | npm packages | Red name, Dim version/author |
| `printSO()` | Stack Overflow | Blue title, Green answered, Cyan tags |

Each result is separated by a `DIM` horizontal rule via `sep()`.

---

## 11. Navigation & Back

```
At any menu:
  [0] or Back option → return -1 from pickMenu() → continue (restart main loop)

At query prompt:
  empty input or "0" → continue (restart main loop)

At Exit (platform menu [5]):
  → break out of while(true) → program ends
```

The main loop is entirely `while(true)` with `continue` for back-navigation and `break` for exit — no recursion, no stack buildup.

---

## 12. Component Diagram

```
┌─────────────────────────────────────────────────────┐
│                      main.cpp                       │
│                                                     │
│  ┌──────────┐   ┌──────────────┐   ┌─────────────┐ │
│  │ pickMenu │   │ printBanner  │   │ print*()    │ │
│  │ (UI/Nav) │   │ cls / sep    │   │ (renderers) │ │
│  └────┬─────┘   └──────────────┘   └──────┬──────┘ │
│       │                                   │        │
│       └──────────── SearchEngine ─────────┘        │
└─────────────────────────┬───────────────────────────┘
                          │
          ┌───────────────┼───────────────┐
          │               │               │
    ┌─────▼──────┐  ┌─────▼──────┐  ┌────▼──────┐
    │PlatformAPIs│  │ HttpClient │  │  nlohmann │
    │ GitHubAPI  │  │  (libcurl) │  │   /json   │
    │ GitLabAPI  │  └─────┬──────┘  └───────────┘
    │ NpmAPI     │        │
    │ StackOverfl│  ┌─────▼──────┐
    └────────────┘  │CacheManager│
                    │ cache.json │
                    └─────┬──────┘
                          │
               ┌──────────▼──────────┐
               │    External APIs    │
               │  api.github.com     │
               │  gitlab.com/api     │
               │  registry.npmjs     │
               │  api.stackexch.     │
               └─────────────────────┘
```

---

## 13. Data Flow Diagram

```
User Input
    │
    ▼
pickMenu() ──[invalid]──► sleep 900ms ──► reprint menu
    │
    │ [valid]
    ▼
normalizeQuery()
    │
    ▼
SearchEngine::search(query, platform, searchType)
    │
    ├──[GitHub]──► GitHubAPI ──► HttpClient::get() ──► CacheManager ──► GitHub REST API
    │                │                                  (hit: skip)      (miss: fetch)
    │                └──[Skills]──► parallel threads ──► /users/{login} × N
    │
    ├──[GitLab]──► GitLabAPI ──► HttpClient::get() ──► CacheManager ──► GitLab API v4
    │
    ├──[npm]────► NpmAPI ────► HttpClient::get() ──► CacheManager ──► npm Registry v1
    │
    └──[SO]─────► StackOverflowAPI ──► HttpClient::get() ──► CacheManager ──► StackExchange API v2.3
                          │
                          ▼
                   JSON response string
                          │
                          ▼
                   nlohmann::json::parse()
                          │
                          ▼
              engine.profiles / repos / packages / questions
                          │
                          ▼
                   print*() renderer
                          │
                          ▼
                  Colored terminal output
```
