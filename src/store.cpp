#include "store.h"

#include <sstream>

bool Store::set(const std::string& key, const std::string& value) {
    std::lock_guard<std::mutex> lock(mutex_);
    data_[key] = value;
    return true;
}

std::pair<bool, std::string> Store::get(const std::string& key) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = data_.find(key);
    if (it != data_.end()) {
        return {true, it->second};
    }
    return {false, ""};
}

std::vector<std::pair<std::string, std::string>> Store::scan(const std::string& prefix) {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<std::pair<std::string, std::string>> results;
    for (auto it = data_.lower_bound(prefix); it != data_.end(); ++it) {
        if (it->first.compare(0, prefix.size(), prefix) != 0) break;
        results.emplace_back(it->first, it->second);
    }
    return results;
}

std::string Store::dump() {
    std::lock_guard<std::mutex> lock(mutex_);
    std::stringstream ss;
    ss << "Total keys: " << data_.size() << "\n";
    
    if (data_.empty()) {
        ss << "(empty)\n";
    } else {
        for (const auto& [key, value] : data_) {
            ss << key << " = " << value << "\n";
        }
    }
    
    return ss.str();
}

size_t Store::size() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return data_.size();
}
