// protocol.cpp
#include "../include/protocol.h"
#include <cstring>

std::vector<char> Protocol::encodeRequest(const Request& req) {
    std::vector<char> buffer;
    
    buffer.push_back(static_cast<char>(req.cmd));
    
    uint16_t key_len = req.key.size();
    buffer.push_back(key_len & 0xFF);
    buffer.push_back((key_len >> 8) & 0xFF);
    buffer.insert(buffer.end(), req.key.begin(), req.key.end());
    
    uint16_t val_len = req.value.size();
    buffer.push_back(val_len & 0xFF);
    buffer.push_back((val_len >> 8) & 0xFF);
    buffer.insert(buffer.end(), req.value.begin(), req.value.end());
    
    return buffer;
}

Request Protocol::decodeRequest(const char* data, size_t len) {
    Request req;
    if (len < 5) return req;
    
    req.cmd = static_cast<Command>(data[0]);
    
    uint16_t key_len = (unsigned char)data[1] | ((unsigned char)data[2] << 8);
    req.key = std::string(data + 3, key_len);
    
    size_t val_offset = 3 + key_len;
    if (val_offset + 2 <= len) {
        uint16_t val_len = (unsigned char)data[val_offset] | ((unsigned char)data[val_offset + 1] << 8);
        req.value = std::string(data + val_offset + 2, val_len);
    }
    
    return req;
}

std::vector<char> Protocol::encodeResponse(const Response& resp) {
    std::vector<char> buffer;
    
    buffer.push_back(static_cast<char>(resp.status));
    
    uint16_t data_len = resp.data.size();
    buffer.push_back(data_len & 0xFF);
    buffer.push_back((data_len >> 8) & 0xFF);
    buffer.insert(buffer.end(), resp.data.begin(), resp.data.end());
    
    return buffer;
}

Response Protocol::decodeResponse(const char* data, size_t len) {
    Response resp;
    if (len < 3) return resp;
    
    resp.status = static_cast<Status>(data[0]);
    
    uint16_t data_len = (unsigned char)data[1] | ((unsigned char)data[2] << 8);
    if (data_len > 0 && len >= 3 + data_len) {
        resp.data = std::string(data + 3, data_len);
    }
    
    return resp;
}
