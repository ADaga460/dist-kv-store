#include "net/connection.h"

#include <spdlog/spdlog.h>

#include <utility>
#include <vector>

#include "net/framing.h"

using asio::ip::tcp;

namespace {

// One client connection. Reads framed requests in a loop and writes framed
// responses, keeping itself alive via shared_from_this for the duration of
// each async operation.
class Session : public std::enable_shared_from_this<Session> {
public:
    Session(tcp::socket socket, TcpServer::Handler& handler)
        : socket_(std::move(socket)), handler_(handler) {}

    void start() {
        spdlog::debug("connection opened from {}", endpoint());
        readHeader();
    }

private:
    std::string endpoint() {
        std::error_code ec;
        auto ep = socket_.remote_endpoint(ec);
        return ec ? "?" : ep.address().to_string() + ":" + std::to_string(ep.port());
    }

    void readHeader() {
        auto self = shared_from_this();
        asio::async_read(
            socket_, asio::buffer(header_),
            [this, self](std::error_code ec, std::size_t) {
                if (ec) return close(ec);
                const uint32_t len = framing::decodeHeader(header_.data());
                if (len == 0 || len > framing::MAX_PAYLOAD) {
                    spdlog::warn("dropping connection: bad frame length {}", len);
                    return;
                }
                body_.resize(len);
                readBody();
            });
    }

    void readBody() {
        auto self = shared_from_this();
        asio::async_read(
            socket_, asio::buffer(body_),
            [this, self](std::error_code ec, std::size_t) {
                if (ec) return close(ec);
                auto req = ProtocolEncoder::decodeRequest(body_.data(), body_.size());
                Response resp = req ? handler_(*req) : Response(Status::ERR);
                write(resp);
            });
    }

    void write(const Response& resp) {
        auto self = shared_from_this();
        auto payload = ProtocolEncoder::encodeResponse(resp);
        auto header = framing::encodeHeader(static_cast<uint32_t>(payload.size()));

        // Own the buffers until the write completes.
        auto out = std::make_shared<std::vector<char>>();
        out->insert(out->end(), header.begin(), header.end());
        out->insert(out->end(), payload.begin(), payload.end());

        asio::async_write(
            socket_, asio::buffer(*out),
            [this, self, out](std::error_code ec, std::size_t) {
                if (ec) return close(ec);
                readHeader();  // next request on this connection
            });
    }

    void close(std::error_code ec) {
        if (ec && ec != asio::error::eof) {
            spdlog::debug("connection closed: {}", ec.message());
        } else {
            spdlog::debug("connection closed");
        }
    }

    tcp::socket socket_;
    TcpServer::Handler& handler_;
    std::array<char, framing::HEADER_SIZE> header_{};
    std::vector<char> body_;
};

}  // namespace

TcpServer::TcpServer(const std::string& host, uint16_t port, size_t worker_threads,
                     Handler handler)
    : acceptor_(io_), worker_threads_(worker_threads), handler_(std::move(handler)) {
    tcp::endpoint endpoint(asio::ip::make_address(host), port);
    acceptor_.open(endpoint.protocol());
    acceptor_.set_option(asio::socket_base::reuse_address(true));
    acceptor_.bind(endpoint);
    acceptor_.listen();
}

void TcpServer::doAccept() {
    acceptor_.async_accept([this](std::error_code ec, tcp::socket socket) {
        if (!ec) {
            std::make_shared<Session>(std::move(socket), handler_)->start();
        } else if (ec != asio::error::operation_aborted) {
            spdlog::warn("accept failed: {}", ec.message());
        }
        if (acceptor_.is_open()) doAccept();
    });
}

void TcpServer::run() {
    doAccept();
    const size_t n = worker_threads_ == 0 ? 1 : worker_threads_;
    workers_.reserve(n - 1);
    for (size_t i = 1; i < n; ++i) {
        workers_.emplace_back([this] { io_.run(); });
    }
    io_.run();  // run on the calling thread too
    for (auto& t : workers_) {
        if (t.joinable()) t.join();
    }
}

void TcpServer::stop() {
    asio::post(io_, [this] {
        std::error_code ec;
        acceptor_.close(ec);
        io_.stop();
    });
}

SyncClient::SyncClient(const std::string& host, uint16_t port) : socket_(io_) {
    tcp::resolver resolver(io_);
    auto endpoints = resolver.resolve(host, std::to_string(port));
    asio::connect(socket_, endpoints);
}

Response SyncClient::send(const Request& req) {
    auto payload = ProtocolEncoder::encodeRequest(req);
    auto header = framing::encodeHeader(static_cast<uint32_t>(payload.size()));

    std::vector<asio::const_buffer> out{asio::buffer(header), asio::buffer(payload)};
    asio::write(socket_, out);

    std::array<char, framing::HEADER_SIZE> resp_header{};
    asio::read(socket_, asio::buffer(resp_header));
    const uint32_t len = framing::decodeHeader(resp_header.data());
    if (len == 0 || len > framing::MAX_PAYLOAD) {
        throw std::runtime_error("invalid response frame length");
    }

    std::vector<char> body(len);
    asio::read(socket_, asio::buffer(body));
    return ProtocolEncoder::decodeResponse(body.data(), body.size());
}
