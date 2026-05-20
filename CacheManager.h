#pragma once
#include <string>
#include <fstream>
#include <chrono>
#include <mutex>
#include <nlohmann/json.hpp>
using namespace std;
using json=nlohmann::json;

class CacheManager{
public:
    static CacheManager& instance(){
        static CacheManager instance;
        return instance;
    }
    string get(const string& key){
        lock_guard<mutex> lock(mtx_);
        load();
        if(!data_.contains(key)) return "";
        long long ts=data_[key]["ts"].get<long long>();
        if(now()-ts>TTL){data_.erase(key);return "";}
        return data_[key]["val"].get<string>();
    }
    void set(const string& key,const string& value){
        lock_guard<mutex> lock(mtx_);
        load();
        data_[key]={{"ts",now()},{"val",value}};
        save();
    }
    void clear(){data_={};save();}
private:
    CacheManager()=default;
    mutex mtx_;
    json data_;
    bool loaded_=false;
    const long long TTL=3600;
    const string FILE="cache.json";
    long long now(){
        return chrono::duration_cast<chrono::seconds>(
            chrono::system_clock::now().time_since_epoch()).count();
    }
    void load(){
        if(loaded_) return;
        loaded_=true;
        ifstream file(FILE);
        if(file.is_open()) try{file>>data_;}catch(...){data_={};}
    }
    void save(){
        ofstream file(FILE);
        if(file.is_open()) file<<data_.dump(2);
    }
};
