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

    // Returns empty string if miss or expired
    string get(const string& key) {
        load();
        if (!data_.contains(key)) return "";
        auto& entry = data_[key];
        long long now = epoch();
        if (now - entry["ts"].get<long long>() > ttl_) {
            data_.erase(key);
            dirty_ = true;
            return "";
        }
        return entry["val"].get<string>();
    }

    void set(const string& key, const string& value) {
        load();
        data_[key] = { {"ts", epoch()}, {"val", value} };
        dirty_ = true;
        save();
    }

    void clear() { data_.clear(); dirty_ = true; save(); }

private:
    CacheManager() = default;
    json data_;
    bool loaded_ = false;
    bool dirty_  = false;
    const long long ttl_ = 3600; // seconds
    const string path_   = "cache.json";

    long long epoch() {
        return chrono::duration_cast<chrono::seconds>(
            chrono::system_clock::now().time_since_epoch()).count();
    }

    void load() {
        if (loaded_) return;
        loaded_ = true;
        ifstream f(path_);
        if (!f.is_open()) return;
        try { f >> data_; } catch(...) { data_ = {}; }
    }

    void save() {
        if (!dirty_) return;
        ofstream f(path_);
        if (f.is_open()) f << data_.dump(2);
        dirty_ = false;
    }
};
