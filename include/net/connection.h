#pragma once
#include <asio.hpp>

#include <functional>
#include <memory>
#include <string>
#include <thread>
#include <vector>

#include "protocol.h"

// Async TCP server built on ASIO. The main thread accepts connections; a pool
// of threads drives a single io_context, so a slow client no longer ties up a
// dedicated worker for its whole session (the old ThreadPool + blocking-recv
// model). Each request is decoded, passed to the handler, and the response is
// framed and written back.
class TcpServer {
public:
    using Handler = std::function<Response(const Request&)>;

    TcpServer(const std::string& host, uint16_t port, size_t worker_threads, Handler handler);

    // Blocks running the io_context across the worker threads until stop().
    void run();
    void stop();

private:
    void doAccept();

    asio::io_context io_;
    asio::ip::tcp::acceptor acceptor_;
    size_t worker_threads_;
    Handler handler_;
    std::vector<std::thread> workers_;
};

// Minimal synchronous client. Presents a blocking send() API on top of an ASIO
// socket, with proper length-prefixed framing so large responses are fully
// read before decoding.
class SyncClient {
public:
    SyncClient(const std::string& host, uint16_t port);
    Response send(const Request& req);

private:
    asio::io_context io_;
    asio::ip::tcp::socket socket_;
};
