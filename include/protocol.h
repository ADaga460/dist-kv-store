#pragma once
#include <string>
#include <vector>
#include <cstdint>

enum class Command : uint8_t {
    SET = 0x01,
    GET = 0x02,
    DUMP = 0x03
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
};

struct Response {
    Status status;
    std::string data;
};

class Protocol {
public:
    static std::vector<char> encodeRequest(const Request& req);
    static std::vector<char> encodeResponse(const Response& resp);
    static Request decodeRequest(const char* data, size_t len);
    static Response decodeResponse(const char* data, size_t len);
};
