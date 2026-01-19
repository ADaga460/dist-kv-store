#include <iostream>
#include <winsock2.h>
#include <string>
#include "../include/protocol.h"

#pragma comment(lib, "ws2_32.lib")

static struct ConsoleInit {
    ConsoleInit() { std::setvbuf(stdout, NULL, _IONBF, 0); }
} console_init;

class SimpleClient {
public:
    SimpleClient(const std::string& host, int port) {
        WSADATA wsa;
        WSAStartup(MAKEWORD(2,2), &wsa);
        
        sock_ = socket(AF_INET, SOCK_STREAM, 0);
        
        sockaddr_in server_addr{};
        server_addr.sin_family = AF_INET;
        server_addr.sin_addr.s_addr = inet_addr(host.c_str());
        server_addr.sin_port = htons(port);
        
        if (connect(sock_, (sockaddr*)&server_addr, sizeof(server_addr)) != 0) {
            throw std::runtime_error("Connection failed");
        }
    }
    
    ~SimpleClient() {
        closesocket(sock_);
        WSACleanup();
    }
    
    Response sendRequest(const Request& req) {
        auto encoded = ProtocolEncoder::encodeRequest(req);
        send(sock_, encoded.data(), encoded.size(), 0);
        
        char buffer[4096];
        int bytes = recv(sock_, buffer, sizeof(buffer), 0);
        
        return ProtocolEncoder::decodeResponse(buffer, bytes);
    }
    
private:
    SOCKET sock_;
};

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cout << "Usage: client <command> [key] [value]" << std::endl;
        std::cout << "  client set <key> <value>" << std::endl;
        std::cout << "  client get <key>" << std::endl;
        std::cout << "  client dump" << std::endl;
        return 1;
    }
    
    try {
        SimpleClient client("127.0.0.1", 8080);
        
        std::string cmd = argv[1];
        Request req;
        
        if (cmd == "set" && argc == 4) {
            req = Request(Command::SET, argv[2], argv[3]);
        } else if (cmd == "get" && argc == 3) {
            req = Request(Command::GET, argv[2]);
        } else if (cmd == "dump") {
            req = Request(Command::DUMP, "");
        } else {
            std::cerr << "Invalid command" << std::endl;
            return 1;
        }
        
        Response resp = client.sendRequest(req);
        
        if (resp.status == Status::OK) {
            if (!resp.data.empty()) {
                std::cout << resp.data;
            } else {
                std::cout << "OK" << std::endl;
            }
        } else if (resp.status == Status::NOT_FOUND) {
            std::cout << "NOT_FOUND" << std::endl;
        } else {
            std::cout << "ERR" << std::endl;
        }
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}
