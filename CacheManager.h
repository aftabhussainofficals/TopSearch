#pragma once
#include <string>
#include <fstream>
#include <chrono>
#include <nlohmann/json.hpp>
using namespace std;
using json = nlohmann::json;

class CacheManager {
public:
    static CacheManager& instance() {
        static CacheManager inst;
        return inst;
    }
    string get(const string& key) {
        load();
        if (!data_.contains(key)) return "";
        long long ts = data_[key]["ts"].get<long long>();
        if (now() - ts > TTL) { data_.erase(key); return ""; }
        return data_[key]["val"].get<string>();
    }
    void set(const string& key, const string& value) {
        load();
        data_[key] = { {"ts", now()}, {"val", value} };
        save();
    }
    void clear() { data_ = {}; save(); }
private:
    CacheManager() = default;
    json data_;
    bool loaded_ = false;
    const long long TTL = 3600;
    const string FILE = "cache.json";
    long long now() {
        return chrono::duration_cast<chrono::seconds>(
            chrono::system_clock::now().time_since_epoch()).count();
    }
    void load() {
        if (loaded_) return;
        loaded_ = true;
        ifstream f(FILE);
        if (f.is_open()) try { f >> data_; } catch (...) { data_ = {}; }
    }
    void save() {
        ofstream f(FILE);
        if (f.is_open()) f << data_.dump(2);
    }
};
