#pragma once
#include <array>
#include <cstdint>
#include <cstring>

#include "protocol.h"

// Wire framing shared by client and server.
//
// Every payload (an encoded Request or Response) is preceded by a 4-byte
// big-endian length prefix. This is what lets us read a *complete* message off
// a TCP stream instead of assuming one recv() == one message, which was the
// framing bug in the original blocking implementation.
namespace framing {

constexpr size_t HEADER_SIZE = 4;

inline std::array<char, HEADER_SIZE> encodeHeader(uint32_t len) {
    std::array<char, HEADER_SIZE> h{};
    h[0] = static_cast<char>((len >> 24) & 0xFF);
    h[1] = static_cast<char>((len >> 16) & 0xFF);
    h[2] = static_cast<char>((len >> 8) & 0xFF);
    h[3] = static_cast<char>(len & 0xFF);
    return h;
}

inline uint32_t decodeHeader(const char* data) {
    return (static_cast<uint32_t>(static_cast<unsigned char>(data[0])) << 24) |
           (static_cast<uint32_t>(static_cast<unsigned char>(data[1])) << 16) |
           (static_cast<uint32_t>(static_cast<unsigned char>(data[2])) << 8) |
           (static_cast<uint32_t>(static_cast<unsigned char>(data[3])));
}

// A payload larger than this is treated as a protocol error and the connection
// is dropped, so a bad/hostile length prefix can't make us allocate wildly.
constexpr uint32_t MAX_PAYLOAD = static_cast<uint32_t>(Protocol::MAX_MESSAGE_SIZE);

}  // namespace framing
