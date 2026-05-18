#include "PlatformAPIs.h"
#include "HttpClient.h"

// GitHub
string GitHubAPI::searchUsers(const string& query){
    return HttpClient::get("https://api.github.com/search/users?q="+query);
}
string GitHubAPI::fetchProfile(const string& username){
    return HttpClient::get("https://api.github.com/users/"+username);
}
string GitHubAPI::searchRepositories(const string& query){
    return HttpClient::get("https://api.github.com/search/repositories?q="+query);
}

// GitLab
string GitLabAPI::searchProjects(const string& query){
    return HttpClient::get("https://gitlab.com/api/v4/projects?visibility=public&search="+query+"&per_page=20&order_by=star_count&sort=desc");
}

// npm
string NpmAPI::searchPackages(const string& query){
    return HttpClient::get("https://registry.npmjs.org/-/v1/search?text="+query+"&size=20");
}

// Stack Overflow
string StackOverflowAPI::searchQuestions(const string& query){
    return HttpClient::get("https://api.stackexchange.com/2.3/search/advanced?q="+query+"&site=stackoverflow&pagesize=20&order=desc&sort=votes&filter=default");
}
