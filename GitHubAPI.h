#pragma once
#include <string>
using namespace std;
class GitHubAPI{
public:
    string searchUsers(const string& query);
    string fetchProfile(const string& username);
    string searchRepositories(const string& query);
};
