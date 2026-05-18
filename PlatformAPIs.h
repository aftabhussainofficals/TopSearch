#pragma once
#include <string>
using namespace std;

class GitHubAPI{
public:
    string searchUsers(const string& query);
    string fetchProfile(const string& username);
    string searchRepositories(const string& query);
};

class GitLabAPI{
public:
    string searchProjects(const string& query);
};

class NpmAPI{
public:
    string searchPackages(const string& query);
};

class StackOverflowAPI{
public:
    string searchQuestions(const string& query);
};
