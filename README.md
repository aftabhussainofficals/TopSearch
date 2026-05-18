# TopSearch

A fast, terminal-based multi-platform search tool built in C++. Search GitHub profiles, repositories, and skills — along with GitLab projects, npm packages, and Stack Overflow questions — all from a single interactive CLI.

---

## Features

### Main Menu
Navigate between all supported platforms from a clean, color-coded terminal interface. Invalid input is automatically cleared and the menu is re-shown.

![Main Menu](images/New%20Folder/mainmenue.jpg)

---

### GitHub — Profiles & Skills
Search GitHub users by name or skill. Optionally rank results by a computed score based on followers, repositories, and following count.

![GitHub Menu](images/New%20Folder/githubmenue.jpg)

---

### GitHub — Repository Download
Search GitHub repositories and download any result as a ZIP archive directly to your machine.

![GitHub Repo Download](images/New%20Folder/githubrepodownload.jpg)

---

### GitLab
Search GitLab public projects with star count, fork count, and direct links.

![GitLab Search](images/New%20Folder/gitlab.jpg)

---

### npm
Search the npm registry for packages with version info, author, description, and a direct link.

![npm Search](images/New%20Folder/npm.jpg)

---

### Stack Overflow
Search Stack Overflow questions with answer status, score, answer count, view count, and tags.

![Stack Overflow Search](images/New%20Folder/stackoverflow.jpg)

---

## Tech Stack

| Component | Details |
|---|---|
| Language | C++17 |
| UI Library | Dear ImGui (vendored) |
| HTTP Client | libcurl |
| JSON Parsing | nlohmann/json |
| Windowing | GLFW + OpenGL3 |
| Build | g++ via MSYS2 (UCRT64) |

---

## Prerequisites

Install dependencies via MSYS2:

```bash
pacman -S mingw-w64-ucrt-x86_64-gcc
pacman -S mingw-w64-ucrt-x86_64-curl
pacman -S mingw-w64-ucrt-x86_64-glfw
pacman -S mingw-w64-ucrt-x86_64-nlohmann-json
```

---

## Build

### Release

```bash
build.bat
```

Output: `TopSearch.exe`

### Debug

```bash
g++ -std=c++17 -g -O0 \
    main.cpp SearchEngine.cpp PlatformAPIs.cpp \
    -I C:/msys64/ucrt64/include \
    -L C:/msys64/ucrt64/lib \
    -lcurl \
    -o TopSearch_debug.exe
```

Output: `TopSearch_debug.exe` — includes debug symbols, compatible with GDB:

```bash
gdb TopSearch_debug.exe
```

---

## Usage

Run the executable:

```bash
TopSearch.exe
```

Then follow the interactive menu:

1. Select a platform: GitHub / GitLab / npm / Stack Overflow
2. For GitHub, choose a search type: Profiles / Repositories / Skills
3. Enter your query
4. Browse results — optionally rank profiles or download repository ZIPs
5. Press `0` or leave query blank to go back at any step

---

## Project Structure

```
TopSearch/
├── main.cpp              # Entry point, UI, menus, result rendering
├── SearchEngine.cpp/h    # Core search dispatch logic
├── GitHubAPI.cpp/h       # GitHub-specific API calls
├── HttpClient.h          # libcurl HTTP wrapper
├── imgui/                # Vendored Dear ImGui source
├── images/               # Screenshots used in this README
├── build.bat             # Build script
└── CMakeLists.txt        # CMake config (alternative build)
```

---

## GitHub Token

The app uses a GitHub personal access token for authenticated API requests (higher rate limits). It is set in `main.cpp`:

```cpp
string HttpClient::githubToken = "<your_github_token>";
```

> Replace with your own token from [github.com/settings/tokens](https://github.com/settings/tokens).

---

## License

This project is open source. Dear ImGui is licensed under the [MIT License](imgui/imgui.h).
