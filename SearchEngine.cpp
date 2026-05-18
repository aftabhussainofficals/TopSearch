#include "SearchEngine.h"
#include "GitHubAPI.h"
#include "HttpClient.h"
#include <thread>
#include <mutex>
#include <algorithm>
using namespace std;
using json=nlohmann::json;
static GitHubAPI api;
static string jsonStr(json obj,string key){
    if(!obj.contains(key)||obj[key].is_null()) return "N/A";
    if(obj[key].is_string()) return obj[key].get<string>();
    return obj[key].dump();
}
static int jsonInt(json obj,string key){
    return(!obj.contains(key)||obj[key].is_null())?0:obj[key].get<int>();
}
static Profile fetchProfileData(const string& username){
    Profile prof;
    prof.username=username;
    string data=api.fetchProfile(username);
    if(data.empty()) return prof;
    try{
        json p=json::parse(data);
        if(p.contains("message")){prof.bio=p["message"].get<string>();return prof;}
        prof.name=jsonStr(p,"name");
        prof.bio=jsonStr(p,"bio");
        prof.followers=jsonInt(p,"followers");
        prof.repos=jsonInt(p,"public_repos");
        prof.following=jsonInt(p,"following");
        prof.score=prof.followers*3+prof.repos*2+prof.following;
    }catch(...){}
    return prof;
}
SearchEngine::SearchEngine(){}
void SearchEngine::search(const string& query,int platformType,int searchType){
    profiles.clear();repos.clear();
    packages.clear();questions.clear();
    statusMessage="Searching...";
    string normalizedQuery=HttpClient::normalizeQuery(query);
    if(platformType==0){
        string result=(searchType==1)?api.searchRepositories(normalizedQuery):api.searchUsers(normalizedQuery);
        if(result.empty()){statusMessage="No data received.";return;}
        try{
            json j=json::parse(result);
            if(!j.contains("items")||j["items"].empty()){statusMessage="No results.";return;}
            int limit=(int)j["items"].size();
            if(searchType==1){
                for(int i=0;i<limit;i++) repos.push_back(j["items"][i]);
                statusMessage="Found "+to_string(limit)+" repositories.";
            }else if(searchType==0){
                for(int i=0;i<limit;i++){
                    Profile p;
                    p.username=j["items"][i]["login"].get<string>();
                    profiles.push_back(p);
                }
                statusMessage="Found "+to_string(profiles.size())+" users.";
            }else{
                string skillQuery=query;
                transform(skillQuery.begin(),skillQuery.end(),skillQuery.begin(),::tolower);
                vector<string> candidates;
                string userResult=api.searchUsers(normalizedQuery);
                if(!userResult.empty()){
                    try{
                        json usersJson=json::parse(userResult);
                        if(usersJson.contains("items"))
                            for(auto& item:usersJson["items"])
                                candidates.push_back(item["login"].get<string>());
                    }catch(...){}
                }
                string repoResult=api.searchRepositories(normalizedQuery);
                if(!repoResult.empty()){
                    try{
                        json reposJson=json::parse(repoResult);
                        if(reposJson.contains("items"))
                            for(auto& item:reposJson["items"]){
                                string login=item["owner"]["login"].get<string>();
                                if(find(candidates.begin(),candidates.end(),login)==candidates.end())
                                    candidates.push_back(login);
                            }
                    }catch(...){}
                }
                if(candidates.empty()){statusMessage="No results.";return;}
                if((int)candidates.size()>30) candidates.resize(30);
                vector<thread> threads;mutex mtx;
                for(auto& uname:candidates){
                    threads.push_back(thread([this,&mtx,uname,skillQuery](){
                        Profile p=fetchProfileData(uname);
                        bool hasBio=p.bio!="N/A"&&!p.bio.empty();
                        string bioLow=p.bio;transform(bioLow.begin(),bioLow.end(),bioLow.begin(),::tolower);
                        string userLow=p.username;transform(userLow.begin(),userLow.end(),userLow.begin(),::tolower);
                        bool match=(hasBio&&bioLow.find(skillQuery)!=string::npos)||userLow.find(skillQuery)!=string::npos;
                        if(match){lock_guard<mutex> lock(mtx);profiles.push_back(p);}
                    }));
                }
                for(auto& t:threads) t.join();
                sort(profiles.begin(),profiles.end(),[&skillQuery](const Profile& a,const Profile& b){
                    auto hasBioMatch=[&skillQuery](const Profile& p){
                        string bioLower=p.bio;transform(bioLower.begin(),bioLower.end(),bioLower.begin(),::tolower);
                        return p.bio!="N/A"&&!p.bio.empty()&&bioLower.find(skillQuery)!=string::npos;
                    };
                    return hasBioMatch(a)>hasBioMatch(b);
                });
                statusMessage="Skill search: "+to_string(profiles.size())+" results.";
            }
        }catch(...){statusMessage="Error parsing response.";}
    }else if(platformType==1){
        string url="https://gitlab.com/api/v4/projects?visibility=public&search="+normalizedQuery+"&per_page=20&order_by=star_count&sort=desc";
        string result=HttpClient::get(url);
        if(result.empty()){statusMessage="No data from GitLab.";return;}
        try{
            json j=json::parse(result);
            if(!j.is_array()||j.empty()){statusMessage="No GitLab results.";return;}
            for(auto& item:j) repos.push_back(item);
            statusMessage="GitLab: Found "+to_string(repos.size())+" projects.";
        }catch(...){statusMessage="Error parsing GitLab response.";}
    }else if(platformType==2){
        string url="https://registry.npmjs.org/-/v1/search?text="+normalizedQuery+"&size=20";
        string result=HttpClient::get(url);
        if(result.empty()){statusMessage="No data from npm.";return;}
        try{
            json j=json::parse(result);
            if(!j.contains("objects")||j["objects"].empty()){statusMessage="No npm results.";return;}
            for(auto& item:j["objects"]) packages.push_back(item);
            statusMessage="npm: Found "+to_string(packages.size())+" packages.";
        }catch(...){statusMessage="Error parsing npm response.";}
    }else if(platformType==3){
        string url="https://api.stackexchange.com/2.3/search/advanced?q="+normalizedQuery+"&site=stackoverflow&pagesize=20&order=desc&sort=votes&filter=default";
        string result=HttpClient::get(url);
        if(result.empty()){statusMessage="No data from Stack Overflow.";return;}
        try{
            json j=json::parse(result);
            if(!j.contains("items")||j["items"].empty()){statusMessage="No Stack Overflow results.";return;}
            for(auto& item:j["items"]) questions.push_back(item);
            statusMessage="Stack Overflow: Found "+to_string(questions.size())+" questions.";
        }catch(...){statusMessage="Error parsing Stack Overflow response.";}
    }
}
