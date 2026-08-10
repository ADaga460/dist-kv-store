#include "protocol.h"

#include <cstring>
#include <stdexcept>

void ProtocolEncoder::writeUint32(std::vector<char>& buf, uint32_t value) {
    buf.push_back(value & 0xFF);
    buf.push_back((value >> 8) & 0xFF);
    buf.push_back((value >> 16) & 0xFF);
    buf.push_back((value >> 24) & 0xFF);
}

uint32_t ProtocolEncoder::readUint32(const char* data) {
    return static_cast<uint32_t>(static_cast<unsigned char>(data[0])) |
           (static_cast<uint32_t>(static_cast<unsigned char>(data[1])) << 8) |
           (static_cast<uint32_t>(static_cast<unsigned char>(data[2])) << 16) |
           (static_cast<uint32_t>(static_cast<unsigned char>(data[3])) << 24);
}

std::vector<char> ProtocolEncoder::encodeRequest(const Request& req) {
    std::vector<char> buffer;
    buffer.reserve(1 + 4 + req.key.size() + 4 + req.value.size());

    buffer.push_back(static_cast<char>(req.cmd));

    if (req.key.size() > Protocol::MAX_KEY_SIZE) {
        throw std::runtime_error("Key too large");
    }
    writeUint32(buffer, static_cast<uint32_t>(req.key.size()));
    buffer.insert(buffer.end(), req.key.begin(), req.key.end());

    if (req.value.size() > Protocol::MAX_VALUE_SIZE) {
        throw std::runtime_error("Value too large");
    }
    writeUint32(buffer, static_cast<uint32_t>(req.value.size()));
    buffer.insert(buffer.end(), req.value.begin(), req.value.end());

    return buffer;
}

Request ProtocolEncoder::decodeRequest(const char* data, size_t len) {
    Request req;
    if (len < 9) return req;

    req.cmd = static_cast<Command>(data[0]);

    uint32_t key_len = readUint32(data + 1);
    if (5 + static_cast<size_t>(key_len) > len) return req;
    req.key = std::string(data + 5, key_len);

    size_t val_offset = 5 + key_len;
    if (val_offset + 4 <= len) {
        uint32_t val_len = readUint32(data + val_offset);
        if (val_offset + 4 + static_cast<size_t>(val_len) <= len) {
            req.value = std::string(data + val_offset + 4, val_len);
        }
    }

    return req;
}

std::vector<char> ProtocolEncoder::encodeResponse(const Response& resp) {
    std::vector<char> buffer;
    buffer.reserve(1 + 4 + resp.data.size());

    buffer.push_back(static_cast<char>(resp.status));
    writeUint32(buffer, static_cast<uint32_t>(resp.data.size()));
    buffer.insert(buffer.end(), resp.data.begin(), resp.data.end());

    return buffer;
}

Response ProtocolEncoder::decodeResponse(const char* data, size_t len) {
    Response resp;
    if (len < 5) return resp;

    resp.status = static_cast<Status>(data[0]);
    uint32_t data_len = readUint32(data + 1);

    if (data_len > 0 && len >= 5 + static_cast<size_t>(data_len)) {
        resp.data = std::string(data + 5, data_len);
    }

    return resp;
}
