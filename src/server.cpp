#include <spdlog/spdlog.h>

#include <csignal>

#include "account.h"
#include "config.h"
#include "net/connection.h"
#include "protocol.h"
#include "store.h"

namespace {
TcpServer* g_server = nullptr;

void onSignal(int) {
    if (g_server) g_server->stop();
}
}  // namespace

int main(int argc, char** argv) {
    Config cfg;
    if (!cfg.parseArgs(argc, argv)) return 1;
    cfg.applyLogLevel();

    Store store;
    seedAccounts(store);

    // Translate a decoded request into a response against the store. This is the
    // single place command handling lives; the transport layer is unaware of it.
    auto handler = [&store](const Request& req) -> Response {
        switch (req.cmd) {
            case Command::SET:
                store.set(req.key, req.value);
                spdlog::info("SET {}", req.key);
                return Response(Status::OK);
            case Command::GET: {
                auto [found, value] = store.get(req.key);
                spdlog::info("GET {} {}", req.key, found ? "hit" : "miss");
                return Response(found ? Status::OK : Status::NOT_FOUND, value);
            }
            case Command::DUMP:
                spdlog::info("DUMP {} keys", store.size());
                return Response(Status::OK, store.dump());
            case Command::SCAN: {
                auto rows = store.scan(req.key);
                spdlog::info("SCAN {} -> {} matches", req.key, rows.size());
                std::string out = "Total matches: " + std::to_string(rows.size()) + "\n";
                for (const auto& [k, v] : rows) out += k + " = " + v + "\n";
                return Response(Status::OK, out);
            }
            default:
                spdlog::warn("unknown command {}", static_cast<int>(req.cmd));
                return Response(Status::ERR);
        }
    };

    try {
        TcpServer server(cfg.listen_host, cfg.listen_port, cfg.worker_threads, handler);
        g_server = &server;
        std::signal(SIGINT, onSignal);
        std::signal(SIGTERM, onSignal);

        spdlog::info("[{}] listening on {}:{} ({} workers)", cfg.node_id, cfg.listen_host,
                     cfg.listen_port, cfg.worker_threads);
        server.run();
        spdlog::info("server stopped");
    } catch (const std::exception& e) {
        spdlog::error("fatal: {}", e.what());
        return 1;
    }
    return 0;
}
