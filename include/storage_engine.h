#pragma once
#include <cstddef>
#include <string>
#include <utility>

class StorageEngine {
public:
    virtual ~StorageEngine() = default;

    virtual bool set(const std::string& key, const std::string& value) = 0;
    virtual std::pair<bool, std::string> get(const std::string& key) = 0;
    virtual std::string dump() = 0;
    virtual size_t size() const = 0;
};