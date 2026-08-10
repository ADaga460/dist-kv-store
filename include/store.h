#pragma once
#include <map>
#include <mutex>
#include <string>
#include <utility>
#include <vector>

#include "storage_engine.h"

class Store : public StorageEngine {
public:
    bool set(const std::string& key, const std::string& value) override;
    std::pair<bool, std::string> get(const std::string& key) override;
    std::vector<std::pair<std::string, std::string>> scan(const std::string& prefix) override;
    std::string dump() override;
    size_t size() const override;

private:
    std::map<std::string, std::string> data_;
    mutable std::mutex mutex_;
};
