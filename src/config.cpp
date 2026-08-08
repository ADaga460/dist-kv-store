#include "config.h"

#include <spdlog/spdlog.h>

#include <fstream>
#include <sstream>
#include <string>

namespace {

std::string trim(const std::string& s) {
    const auto begin = s.find_first_not_of(" \t\r\n");
    if (begin == std::string::npos) return "";
    const auto end = s.find_last_not_of(" \t\r\n");
    return s.substr(begin, end - begin + 1);
}

std::vector<std::string> splitCsv(const std::string& s) {
    std::vector<std::string> out;
    std::stringstream ss(s);
    std::string item;
    while (std::getline(ss, item, ',')) {
        item = trim(item);
        if (!item.empty()) out.push_back(item);
    }
    return out;
}

// Assign a single key/value pair onto the config. Unknown keys are ignored so
// that a shared config file can carry keys other phases care about.
void assign(Config& c, const std::string& key, const std::string& value) {
    if (key == "node_id") c.node_id = value;
    else if (key == "listen_host") c.listen_host = value;
    else if (key == "listen_port") c.listen_port = static_cast<uint16_t>(std::stoi(value));
    else if (key == "peers") c.peers = splitCsv(value);
    else if (key == "data_dir") c.data_dir = value;
    else if (key == "worker_threads") c.worker_threads = static_cast<size_t>(std::stoul(value));
    else if (key == "log_level") c.log_level = value;
    else if (key == "connect_host") c.connect_host = value;
    else if (key == "connect_port") c.connect_port = static_cast<uint16_t>(std::stoi(value));
}

}  // namespace

bool Config::loadFile(const std::string& path) {
    std::ifstream in(path);
    if (!in) return false;

    std::string line;
    while (std::getline(in, line)) {
        line = trim(line);
        if (line.empty() || line[0] == '#') continue;
        const auto eq = line.find('=');
        if (eq == std::string::npos) continue;
        assign(*this, trim(line.substr(0, eq)), trim(line.substr(eq + 1)));
    }
    return true;
}

bool Config::parseArgs(int argc, char** argv) {
    // First pass: honour --config so file values act as defaults under flags.
    for (int i = 1; i < argc - 1; ++i) {
        if (std::string(argv[i]) == "--config") {
            if (!loadFile(argv[i + 1])) {
                spdlog::warn("could not open config file: {}", argv[i + 1]);
            }
        }
    }

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg.rfind("--", 0) != 0) continue;
        arg = arg.substr(2);

        std::string key, value;
        const auto eq = arg.find('=');
        if (eq != std::string::npos) {
            key = arg.substr(0, eq);
            value = arg.substr(eq + 1);
        } else {
            key = arg;
            if (key == "config") { ++i; continue; }  // already handled
            if (i + 1 >= argc) {
                spdlog::error("flag --{} is missing a value", key);
                return false;
            }
            value = argv[++i];
        }
        assign(*this, key, value);
    }
    return true;
}

void Config::applyLogLevel() const {
    spdlog::set_level(spdlog::level::from_str(log_level));
}
