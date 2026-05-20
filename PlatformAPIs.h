#pragma once
#include <string>
using namespace std;

class BasePlatformAPI{
public:
    virtual string search(const string& query)=0;
    virtual ~BasePlatformAPI()=default;
};

class GitHubAPI : public BasePlatformAPI{
public:
    string search(const string& query) override;
    string searchUsers(const string& query);
    string fetchProfile(const string& username);
    string searchRepositories(const string& query);
};

class GitLabAPI : public BasePlatformAPI{
public:
    string search(const string& query) override;
    string searchProjects(const string& query);
};

class NpmAPI : public BasePlatformAPI{
public:
    string search(const string& query) override;
    string searchPackages(const string& query);
};

class StackOverflowAPI : public BasePlatformAPI{
public:
    string search(const string& query) override;
    string searchQuestions(const string& query);
};
