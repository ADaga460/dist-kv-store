// server.cpp
#include <iostream>
#include <winsock2.h>
#include "../include/store.h"
#include "../include/protocol.h"
#include "../include/threadpool.h"

#pragma comment(lib, "ws2_32.lib")

// Force unbuffered output for Windows
static struct ConsoleInit {
    ConsoleInit() {
        std::setvbuf(stdout, NULL, _IONBF, 0);
    }
} console_init;

void handleClient(SOCKET client_sock, Store& store) {
    char buffer[4096];
    
    std::cout << "[SERVER] Thread " << std::this_thread::get_id() 
              << " handling client (socket: " << client_sock << ")" << std::endl;
    
    while (true) {
        int bytes = recv(client_sock, buffer, sizeof(buffer), 0);
        
        if (bytes <= 0) {
            std::cout << "[SERVER] Client disconnected (thread " 
                      << std::this_thread::get_id() << ")" << std::endl;
            break;
        }
        
        std::cout << "[SERVER] Thread " << std::this_thread::get_id() 
                  << " received " << bytes << " bytes" << std::endl;
        
        Request req = Protocol::decodeRequest(buffer, bytes);
        Response resp;
        
        if (req.cmd == Command::SET) {
            std::cout << "[SERVER] Thread " << std::this_thread::get_id() 
                      << " SET key='" << req.key << "' value='" << req.value << "'" << std::endl;
            store.set(req.key, req.value);
            resp.status = Status::OK;
            
        } else if (req.cmd == Command::GET) {
            std::cout << "[SERVER] Thread " << std::this_thread::get_id() 
                      << " GET key='" << req.key << "'" << std::endl;
            auto [found, value] = store.get(req.key);
            if (found) {
                resp.status = Status::OK;
                resp.data = value;
            } else {
                resp.status = Status::NOT_FOUND;
            }
            
        } else if (req.cmd == Command::DUMP) {
            std::cout << "[SERVER] Thread " << std::this_thread::get_id() 
                      << " DUMP" << std::endl;
            resp.status = Status::OK;
            resp.data = store.dump();
            
        } else {
            std::cout << "[SERVER] Unknown command: " << (int)req.cmd << std::endl;
            resp.status = Status::ERR;
        }
        
        auto encoded = Protocol::encodeResponse(resp);
        int sent = send(client_sock, encoded.data(), encoded.size(), 0);
        std::cout << "[SERVER] Thread " << std::this_thread::get_id() 
                  << " sent " << sent << " bytes" << std::endl;
    }
    
    closesocket(client_sock);
}

int main() {
    std::cout << "=== Distributed KV Store Server (Multi-threaded) ===" << std::endl;
    std::cout << "[INIT] Starting Winsock..." << std::endl;
    
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2,2), &wsa) != 0) {
        std::cerr << "[ERROR] WSAStartup failed" << std::endl;
        return 1;
    }
    
    std::cout << "[INIT] Creating socket..." << std::endl;
    SOCKET server_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (server_sock == INVALID_SOCKET) {
        std::cerr << "[ERROR] Socket creation failed" << std::endl;
        return 1;
    }
    
    sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(8080);
    
    std::cout << "[INIT] Binding to port 8080..." << std::endl;
    if (bind(server_sock, (sockaddr*)&server_addr, sizeof(server_addr)) == SOCKET_ERROR) {
        std::cerr << "[ERROR] Bind failed" << std::endl;
        return 1;
    }
    
    std::cout << "[INIT] Listening for connections..." << std::endl;
    listen(server_sock, 10);
    
    // 4 worker threads to start
    ThreadPool pool(4);
    Store store;
    
    std::cout << "[READY] Server listening on port 8080 (4 worker threads)" << std::endl;
    std::cout << "========================================" << std::endl;
    
    while (true) {
        std::cout << "\n[SERVER] Waiting for client... (current keys: " 
                  << store.size() << ")" << std::endl;
        
        SOCKET client_sock = accept(server_sock, NULL, NULL);
        
        std::cout << "[SERVER] Client accepted, submitting to thread pool" << std::endl;
        
        // cubmit client to pool
        pool.submit([client_sock, &store]() {
            handleClient(client_sock, store);
        });
    }
    
    closesocket(server_sock);
    WSACleanup();
    return 0;
}
