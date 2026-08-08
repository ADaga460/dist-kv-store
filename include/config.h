#pragma once
#include <string>
#include <vector>
#include <cstdint>

// Runtime configuration for a node. Populated from a config file and/or
// command-line flags, replacing the values that used to be hardcoded in
// server.cpp / client.cpp.
struct Config {
    std::string node_id = "node-1";
    std::string listen_host = "0.0.0.0";
    uint16_t listen_port = 8080;
    std::vector<std::string> peers;      // "host:port" entries (used from Phase 4 on)
    std::string data_dir = "./data";
    size_t worker_threads = 4;
    std::string log_level = "info";      // trace|debug|info|warn|error

    // Client-only convenience: where the CLI client connects to.
    std::string connect_host = "127.0.0.1";
    uint16_t connect_port = 8080;

    // Load "key = value" lines from a file. Unknown keys are ignored.
    // Returns false if the path was given but could not be opened.
    bool loadFile(const std::string& path);

    // Apply "--key value" / "--key=value" flags from argv. Recognises a
    // leading "--config <path>" and loads that file first. Returns false on a
    // malformed flag.
    bool parseArgs(int argc, char** argv);

    // Apply "log_level" to the global spdlog logger.
    void applyLogLevel() const;
};
