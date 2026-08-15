#pragma once
#include <string>
#include <vector>
#include <cstdint>
#include <optional>

// constants
namespace Protocol {
    constexpr size_t MAX_KEY_SIZE = 256;
    constexpr size_t MAX_VALUE_SIZE = 1024 * 64;  // 64KB
    constexpr size_t MAX_MESSAGE_SIZE = 1024 * 128;  // 128KB
}

enum class Command : uint8_t {
    SET = 0x01,
    GET = 0x02,
    DUMP = 0x03,
    SCAN = 0x04,
    DEPOSIT = 0x05,
    WITHDRAW = 0x06
};

enum class Status : uint8_t {
    OK = 0x00,
    NOT_FOUND = 0x01,
    ERR = 0x02
};

struct Request {
    Command cmd;
    std::string key;
    std::string value;
    
    Request() : cmd(Command::GET) {}
    Request(Command c, const std::string& k, const std::string& v = "") 
        : cmd(c), key(k), value(v) {}
};

struct Response {
    Status status;
    std::string data;
    
    Response() : status(Status::ERR) {}
    Response(Status s, const std::string& d = "") 
        : status(s), data(d) {}
};

class ProtocolEncoder {
public:
    static std::vector<char> encodeRequest(const Request& req);
    static std::vector<char> encodeResponse(const Response& resp);
    static std::optional<Request> decodeRequest(const char* data, size_t len);
    static Response decodeResponse(const char* data, size_t len);
    
private:
    static void writeUint32(std::vector<char>& buf, uint32_t value);
    static uint32_t readUint32(const char* data);
};
