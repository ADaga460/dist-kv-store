#pragma once
#include <unordered_map>
#include <string>

class Store {
public:
    bool set(const std::string& key, const std::string& value);
    std::pair<bool, std::string> get(const std::string& key);
    std::string dump();  // New: return all key-value pairs
    size_t size() const; // New: return count
    
private:
    std::unordered_map<std::string, std::string> data_;
};
