#include <spdlog/spdlog.h>

#include <iostream>
#include <string>

#include "config.h"
#include "net/connection.h"
#include "protocol.h"

namespace {

void printUsage() {
    std::cout << "Usage: client [--connect_host H] [--connect_port P] <command> [key] [value]\n"
              << "  client set <key> <value>\n"
              << "  client get <key>\n"
              << "  client scan <prefix>\n"
              << "  client dump\n";
}

}  // namespace

int main(int argc, char** argv) {
    Config cfg;
    // The client shares the flag parser but only cares about connect_host/port.
    // Collect positional (non-flag) args separately.
    std::vector<std::string> pos;
    for (int i = 1; i < argc; ++i) {
        std::string a = argv[i];
        if (a.rfind("--", 0) == 0) {
            // consume "--key value" form (skip the value too)
            if (a.find('=') == std::string::npos && i + 1 < argc) ++i;
            continue;
        }
        pos.push_back(a);
    }
    cfg.parseArgs(argc, argv);
    cfg.applyLogLevel();

    if (pos.empty()) {
        printUsage();
        return 1;
    }

    Request req;
    const std::string& cmd = pos[0];
    if (cmd == "set" && pos.size() == 3) {
        req = Request(Command::SET, pos[1], pos[2]);
    } else if (cmd == "get" && pos.size() == 2) {
        req = Request(Command::GET, pos[1]);
    } else if (cmd == "scan" && pos.size() == 2) {
        req = Request(Command::SCAN, pos[1]);
    } else if (cmd == "dump") {
        req = Request(Command::DUMP, "");
    } else {
        printUsage();
        return 1;
    }

    try {
        SyncClient client(cfg.connect_host, cfg.connect_port);
        Response resp = client.send(req);

        switch (resp.status) {
            case Status::OK:
                if (!resp.data.empty()) std::cout << resp.data;
                else std::cout << "OK\n";
                break;
            case Status::NOT_FOUND:
                std::cout << "NOT_FOUND\n";
                break;
            default:
                std::cout << "ERR\n";
                return 1;
        }
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }
    return 0;
}
