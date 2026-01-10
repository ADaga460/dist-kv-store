// store.cpp
#include "../include/store.h"
#include <sstream>
#include <iostream>

bool Store::set(const std::string& key, const std::string& value) {
    data_[key] = value;
    std::cout << "[STORE] SET: " << key << " = " << value << std::endl;
    return true;
}

std::pair<bool, std::string> Store::get(const std::string& key) {
    auto it = data_.find(key);
    if (it != data_.end()) {
        std::cout << "[STORE] GET: " << key << " -> " << it->second << std::endl;
        return {true, it->second};
    }
    std::cout << "[STORE] GET: " << key << " -> NOT_FOUND" << std::endl;
    return {false, ""};
}

std::string Store::dump() {
    std::stringstream ss;
    ss << "Total keys: " << data_.size() << "\n";
    
    if (data_.empty()) {
        ss << "(empty)\n";
    } else {
        for (const auto& [key, value] : data_) {
            ss << key << " = " << value << "\n";
        }
    }
    
    std::cout << "[STORE] DUMP: Returning " << data_.size() << " keys" << std::endl;
    return ss.str();
}

size_t Store::size() const {
    return data_.size();
}
