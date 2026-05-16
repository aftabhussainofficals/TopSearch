#include "GitHubAPI.h"
#include "HttpClient.h"
string GitHubAPI::searchUsers(const string& query) {
    return HttpClient::get("https://api.github.com/search/users?q=" + query);
}
string GitHubAPI::fetchProfile(const string& query) {
    return HttpClient::get("https://api.github.com/users/" + query);
}
string GitHubAPI::searchRepositories(const string& query) {
    return HttpClient::get("https://api.github.com/search/repositories?q=" + query);
}
