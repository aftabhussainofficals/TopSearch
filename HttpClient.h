#pragma once
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <string>
#include <curl/curl.h>
using namespace std;
class HttpClient {
public:
    static size_t WriteCallback(void* contents, size_t size, size_t nmemb, string* data) {
        data->append((char*)contents, size * nmemb);
        return size * nmemb;
    }
    static string githubToken; 

    static string get(const string& url) {
        string response;
        CURL* curl = curl_easy_init();
        if (curl) {
            struct curl_slist* headers = NULL;
            headers = curl_slist_append(headers, "User-Agent: TopSearch");
            if (!githubToken.empty()) {
                string auth = "Authorization: token " + githubToken;
                headers = curl_slist_append(headers, auth.c_str());
            }
            curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
            curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
            curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
            curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
            curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
            curl_easy_perform(curl);
            curl_easy_cleanup(curl);
            curl_slist_free_all(headers);
        }
        return response;
    }
    static bool downloadFile(const string& url, const string& filename) {
        CURL* curl = curl_easy_init();
        if (!curl) return false;
        FILE* fp = fopen(filename.c_str(), "wb");
        if (!fp) return false;
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 2L);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, fwrite);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);
        curl_easy_setopt(curl, CURLOPT_FAILONERROR, 1L);
        struct curl_slist* headers = NULL;
        headers = curl_slist_append(headers, "User-Agent: TopSearch");
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        CURLcode res = curl_easy_perform(curl);
        curl_easy_cleanup(curl);
        curl_slist_free_all(headers);
        fclose(fp);
        return (res == CURLE_OK);
    }
    static string normalizeQuery(string q) {
        string cleaned;
        for (char c : q) {
            if (c == ' ') continue;
            cleaned += tolower(c);
        }
        return cleaned;
    }
};
