#include "SearchEngine.h"
#include "GitHubAPI.h"
#include "HttpClient.h"
#include <thread>
#include <mutex>
#include <algorithm>
using namespace std;
using json=nlohmann::json;
static GitHubAPI api;
static string safe(json obj,string key){
    if(!obj.contains(key)||obj[key].is_null()) return "N/A";
    if(obj[key].is_string()) return obj[key].get<string>();
    return obj[key].dump();
}
static int safeInt(json obj,string key){
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
        prof.name=safe(p,"name");
        prof.bio=safe(p,"bio");
        prof.followers=safeInt(p,"followers");
        prof.repos=safeInt(p,"public_repos");
        prof.following=safeInt(p,"following");
        prof.score=prof.followers*3+prof.repos*2+prof.following;
    }catch(...){}
    return prof;
}
SearchEngine::SearchEngine(){}
void SearchEngine::search(const string& query,int platformType,int searchType){
    profiles.clear();repos.clear();
    packages.clear();questions.clear();
    statusMessage="Searching...";
    string cq=HttpClient::normalizeQuery(query);
    if(platformType==0){
        string result=(searchType==1)?api.searchRepositories(cq):api.searchUsers(cq);
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
                string sq=query;
                transform(sq.begin(),sq.end(),sq.begin(),::tolower);
                vector<string> candidates;
                string userResult=api.searchUsers(cq);
                if(!userResult.empty()){
                    try{
                        json uj=json::parse(userResult);
                        if(uj.contains("items"))
                            for(auto& item:uj["items"])
                                candidates.push_back(item["login"].get<string>());
                    }catch(...){}
                }
                string repoResult=api.searchRepositories(cq);
                if(!repoResult.empty()){
                    try{
                        json rj=json::parse(repoResult);
                        if(rj.contains("items"))
                            for(auto& item:rj["items"]){
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
                    threads.push_back(thread([this,&mtx,uname,sq](){
                        Profile p=fetchProfileData(uname);
                        bool hasBio=p.bio!="N/A"&&!p.bio.empty();
                        string bioLow=p.bio;transform(bioLow.begin(),bioLow.end(),bioLow.begin(),::tolower);
                        string userLow=p.username;transform(userLow.begin(),userLow.end(),userLow.begin(),::tolower);
                        bool match=(hasBio&&bioLow.find(sq)!=string::npos)||userLow.find(sq)!=string::npos;
                        if(match){lock_guard<mutex> lock(mtx);profiles.push_back(p);}
                    }));
                }
                for(auto& t:threads) t.join();
                sort(profiles.begin(),profiles.end(),[&sq](const Profile& a,const Profile& b){
                    auto hasBioMatch=[&sq](const Profile& p){
                        string bl=p.bio;transform(bl.begin(),bl.end(),bl.begin(),::tolower);
                        return p.bio!="N/A"&&!p.bio.empty()&&bl.find(sq)!=string::npos;
                    };
                    return hasBioMatch(a)>hasBioMatch(b);
                });
                statusMessage="Skill search: "+to_string(profiles.size())+" results.";
            }
        }catch(...){statusMessage="Error parsing response.";}
    }else if(platformType==1){
        string url="https://gitlab.com/api/v4/projects?visibility=public&search="+cq+"&per_page=20&order_by=star_count&sort=desc";
        string result=HttpClient::get(url);
        if(result.empty()){statusMessage="No data from GitLab.";return;}
        try{
            json j=json::parse(result);
            if(!j.is_array()||j.empty()){statusMessage="No GitLab results.";return;}
            for(auto& item:j) repos.push_back(item);
            statusMessage="GitLab: Found "+to_string(repos.size())+" projects.";
        }catch(...){statusMessage="Error parsing GitLab response.";}
    }else if(platformType==2){
        string url="https://registry.npmjs.org/-/v1/search?text="+cq+"&size=20";
        string result=HttpClient::get(url);
        if(result.empty()){statusMessage="No data from npm.";return;}
        try{
            json j=json::parse(result);
            if(!j.contains("objects")||j["objects"].empty()){statusMessage="No npm results.";return;}
            for(auto& item:j["objects"]) packages.push_back(item);
            statusMessage="npm: Found "+to_string(packages.size())+" packages.";
        }catch(...){statusMessage="Error parsing npm response.";}
    }else if(platformType==3){
        string url="https://api.stackexchange.com/2.3/search/advanced?q="+cq+"&site=stackoverflow&pagesize=20&order=desc&sort=votes&filter=default";
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
