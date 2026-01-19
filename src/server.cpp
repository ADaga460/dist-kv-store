#include <iostream>
#include <winsock2.h>
#include "../include/store.h"
#include "../include/protocol.h"
#include "../include/threadpool.h"

#pragma comment(lib, "ws2_32.lib")

static struct ConsoleInit {
    ConsoleInit() { std::setvbuf(stdout, NULL, _IONBF, 0); }
} console_init;

void handleClient(SOCKET client_sock, Store& store) {
    char buffer[4096];
    auto thread_id = std::this_thread::get_id();
    
    std::cout << "[CONN] Thread " << thread_id << " connected" << std::endl;
    
    while (true) {
        int bytes = recv(client_sock, buffer, sizeof(buffer), 0);
        if (bytes <= 0) break;
        
        Request req = ProtocolEncoder::decodeRequest(buffer, bytes);
        Response resp;
        
        switch (req.cmd) {
            case Command::SET:
                store.set(req.key, req.value);
                resp.status = Status::OK;
                std::cout << "[SET] " << req.key << std::endl;
                break;
                
            case Command::GET: {
                auto [found, value] = store.get(req.key);
                resp = Response(found ? Status::OK : Status::NOT_FOUND, value);
                std::cout << "[GET] " << req.key << (found ? " ✓" : " ✗") << std::endl;
                break;
            }
            
            case Command::DUMP:
                resp = Response(Status::OK, store.dump());
                std::cout << "[DUMP] " << store.size() << " keys" << std::endl;
                break;
                
            default:
                resp.status = Status::ERR;
        }
        
        auto encoded = ProtocolEncoder::encodeResponse(resp);
        send(client_sock, encoded.data(), encoded.size(), 0);
    }
    
    std::cout << "[DISC] Thread " << thread_id << " disconnected" << std::endl;
    closesocket(client_sock);
}

int main() {
    std::cout << "=== KV Store Server (Multi-threaded) ===" << std::endl;
    
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2,2), &wsa) != 0) {
        std::cerr << "WSAStartup failed" << std::endl;
        return 1;
    }
    
    SOCKET server_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (server_sock == INVALID_SOCKET) {
        std::cerr << "Socket creation failed" << std::endl;
        return 1;
    }
    
    sockaddr_in server_addr{};
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(8080);
    
    if (bind(server_sock, (sockaddr*)&server_addr, sizeof(server_addr)) == SOCKET_ERROR) {
        std::cerr << "Bind failed" << std::endl;
        return 1;
    }
    
    listen(server_sock, 10);
    
    ThreadPool pool(4);
    Store store;
    
    std::cout << "[READY] Listening on port 8080 (4 workers)" << std::endl;
    
    while (true) {
        SOCKET client_sock = accept(server_sock, NULL, NULL);
        pool.submit([client_sock, &store]() {
            handleClient(client_sock, store);
        });
    }
    
    closesocket(server_sock);
    WSACleanup();
    return 0;
}
