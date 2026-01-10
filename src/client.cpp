// client.cpp
#include <iostream>
#include <winsock2.h>
#include <string>
#include "../include/protocol.h"

#pragma comment(lib, "ws2_32.lib")

static struct ConsoleInit {
    ConsoleInit() {
        std::cout.sync_with_stdio(false);
        std::setvbuf(stdout, NULL, _IONBF, 0);
    }
} console_init;

int main(int argc, char** argv) {

    if (argc < 2) {
        std::cout << "Usage: client <command> [key] [value]" << std::endl << std::flush;
        std::cout << "  client set mykey myvalue" << std::endl << std::flush;
        std::cout << "  client get mykey" << std::endl << std::flush;
        std::cout << "  client dump" << std::endl << std::flush;
        return 1;
    }
    
    std::cout << "[CLIENT] Starting..." << std::endl << std::flush;
    
    WSADATA wsa;
    WSAStartup(MAKEWORD(2,2), &wsa);
    
    std::cout << "[CLIENT] Connecting to 127.0.0.1:8080..." << std::endl << std::flush;
    SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);
    
    sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    server_addr.sin_port = htons(8080);
    
    if (connect(sock, (sockaddr*)&server_addr, sizeof(server_addr)) != 0) {
        std::cerr << "[ERROR] Failed to connect to server" << std::endl << std::flush;
        return 1;
    }
    
    std::cout << "[CLIENT] Connected" << std::endl << std::flush;
    
    Request req;
    std::string cmd = argv[1];
    
    if (cmd == "set") {
        if (argc < 4) {
            std::cerr << "[ERROR] SET requires key and value" << std::endl << std::flush;
            return 1;
        }
        req.cmd = Command::SET;
        req.key = argv[2];
        req.value = argv[3];
        std::cout << "[CLIENT] SET " << req.key << " = " << req.value << std::endl << std::flush;
        
    } else if (cmd == "get") {
        if (argc < 3) {
            std::cerr << "[ERROR] GET requires key" << std::endl << std::flush;
            return 1;
        }
        req.cmd = Command::GET;
        req.key = argv[2];
        std::cout << "[CLIENT] GET " << req.key << std::endl << std::flush;
        
    } else if (cmd == "dump") {
        req.cmd = Command::DUMP;
        std::cout << "[CLIENT] DUMP (listing all keys)" << std::endl    << std::flush;
        
    } else {
        std::cerr << "[ERROR] Unknown command: " << cmd << std::endl << std::flush;
        return 1;
    }
    
    auto encoded = Protocol::encodeRequest(req);
    std::cout << "[CLIENT] Sending " << encoded.size() << " bytes..." << std::endl << std::flush;
    send(sock, encoded.data(), encoded.size(), 0);
    
    char buffer[4096];
    int bytes = recv(sock, buffer, sizeof(buffer), 0);
    std::cout << "[CLIENT] Received " << bytes << " bytes" << std::endl << std::flush;
    
    Response resp = Protocol::decodeResponse(buffer, bytes);
    
    std::cout << "[CLIENT] Response status: " << (int)resp.status << std::endl << std::flush;
    std::cout << "--------" << std::endl << std::flush;
    
    if (resp.status == Status::OK) {
        if (!resp.data.empty()) {
            std::cout << resp.data;
        } else {
            std::cout << "OK" << std::endl << std::flush;
        }
    } else if (resp.status == Status::NOT_FOUND) {
        std::cout << "NOT_FOUND" << std::endl << std::flush;
    } else {
        std::cout << "ERR" << std::endl << std::flush;
    }
    
    closesocket(sock);
    WSACleanup();
    return 0;
}
