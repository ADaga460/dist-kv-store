#pragma once
#include <unordered_map>
#include <string>
#include <mutex>

class Store {
public:
    bool set(const std::string& key, const std::string& value);
    std::pair<bool, std::string> get(const std::string& key);
    std::string dump();
    size_t size() const;
    
private:
    std::unordered_map<std::string, std::string> data_;
    mutable std::mutex mutex_;
};
