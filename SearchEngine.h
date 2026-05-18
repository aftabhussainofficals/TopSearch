#pragma once
#include <string>
#include <vector>
#include <nlohmann/json.hpp>
struct Profile{
    std::string username;
    std::string name;
    std::string bio;
    int followers=0,repos=0,following=0,score=0;
};
class SearchEngine{
public:
    SearchEngine();
    void search(const std::string& query,int platformType,int searchType);
    std::vector<Profile> profiles;
    std::vector<nlohmann::json> repos;
    std::vector<nlohmann::json> packages;
    std::vector<nlohmann::json> questions;
    std::string statusMessage;
};
