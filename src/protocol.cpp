#include "../include/protocol.h"
#include <cstring>
#include <stdexcept>

void ProtocolEncoder::writeUint16(std::vector<char>& buf, uint16_t value) {
    buf.push_back(value & 0xFF);
    buf.push_back((value >> 8) & 0xFF);
}

uint16_t ProtocolEncoder::readUint16(const char* data) {
    return static_cast<uint16_t>(static_cast<unsigned char>(data[0])) | 
           (static_cast<uint16_t>(static_cast<unsigned char>(data[1])) << 8);
}

std::vector<char> ProtocolEncoder::encodeRequest(const Request& req) {
    std::vector<char> buffer;
    buffer.reserve(1 + 2 + req.key.size() + 2 + req.value.size());
    
    // Command byte
    buffer.push_back(static_cast<char>(req.cmd));
    
    // Key length + key
    if (req.key.size() > Protocol::MAX_KEY_SIZE) {
        throw std::runtime_error("Key too large");
    }
    writeUint16(buffer, static_cast<uint16_t>(req.key.size()));
    buffer.insert(buffer.end(), req.key.begin(), req.key.end());
    
    // Value length + value
    if (req.value.size() > Protocol::MAX_VALUE_SIZE) {
        throw std::runtime_error("Value too large");
    }
    writeUint16(buffer, static_cast<uint16_t>(req.value.size()));
    buffer.insert(buffer.end(), req.value.begin(), req.value.end());
    
    return buffer;
}

Request ProtocolEncoder::decodeRequest(const char* data, size_t len) {
    Request req;
    if (len < 5) return req;  // Minimum: cmd(1) + key_len(2) + val_len(2)
    
    req.cmd = static_cast<Command>(data[0]);
    
    uint16_t key_len = readUint16(data + 1);
    if (3 + key_len > len) return req;
    req.key = std::string(data + 3, key_len);
    
    size_t val_offset = 3 + key_len;
    if (val_offset + 2 <= len) {
        uint16_t val_len = readUint16(data + val_offset);
        if (val_offset + 2 + val_len <= len) {
            req.value = std::string(data + val_offset + 2, val_len);
        }
    }
    
    return req;
}

std::vector<char> ProtocolEncoder::encodeResponse(const Response& resp) {
    std::vector<char> buffer;
    buffer.reserve(1 + 2 + resp.data.size());
    
    buffer.push_back(static_cast<char>(resp.status));
    writeUint16(buffer, static_cast<uint16_t>(resp.data.size()));
    buffer.insert(buffer.end(), resp.data.begin(), resp.data.end());
    
    return buffer;
}

Response ProtocolEncoder::decodeResponse(const char* data, size_t len) {
    Response resp;
    if (len < 3) return resp;
    
    resp.status = static_cast<Status>(data[0]);
    uint16_t data_len = readUint16(data + 1);
    
    if (data_len > 0 && len >= 3 + data_len) {
        resp.data = std::string(data + 3, data_len);
    }
    
    return resp;
}
