#include "PlatformAPIs.h"
#include "HttpClient.h"

// GitHubAPI
string GitHubAPI::search(const string& query){
    return searchUsers(query);
}
string GitHubAPI::searchUsers(const string& query){
    return HttpClient::get("https://api.github.com/search/users?q="+query);
}
string GitHubAPI::fetchProfile(const string& username){
    return HttpClient::get("https://api.github.com/users/"+username);
}
string GitHubAPI::searchRepositories(const string& query){
    return HttpClient::get("https://api.github.com/search/repositories?q="+query);
}

// GitLabAPI
string GitLabAPI::search(const string& query){
    return searchProjects(query);
}
string GitLabAPI::searchProjects(const string& query){
    return HttpClient::get("https://gitlab.com/api/v4/projects?visibility=public&search="+query+"&per_page=20&order_by=star_count&sort=desc");
}

// NpmAPI
string NpmAPI::search(const string& query){
    return searchPackages(query);
}
string NpmAPI::searchPackages(const string& query){
    return HttpClient::get("https://registry.npmjs.org/-/v1/search?text="+query+"&size=20");
}

// StackOverflowAPI
string StackOverflowAPI::search(const string& query){
    return searchQuestions(query);
}
string StackOverflowAPI::searchQuestions(const string& query){
    return HttpClient::get("https://api.stackexchange.com/2.3/search/advanced?q="+query+"&site=stackoverflow&pagesize=20&order=desc&sort=votes&filter=default");
}
