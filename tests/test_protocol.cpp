#include <gtest/gtest.h>

#include <string>
#include <vector>

#include "protocol.h"

TEST(ProtocolTest, RequestRoundTripSet) {
    Request in(Command::SET, "mykey", "myvalue");
    auto bytes = ProtocolEncoder::encodeRequest(in);
    Request out = ProtocolEncoder::decodeRequest(bytes.data(), bytes.size());

    EXPECT_EQ(out.cmd, Command::SET);
    EXPECT_EQ(out.key, "mykey");
    EXPECT_EQ(out.value, "myvalue");
}

TEST(ProtocolTest, RequestRoundTripGetHasNoValue) {
    Request in(Command::GET, "mykey");
    auto bytes = ProtocolEncoder::encodeRequest(in);
    Request out = ProtocolEncoder::decodeRequest(bytes.data(), bytes.size());

    EXPECT_EQ(out.cmd, Command::GET);
    EXPECT_EQ(out.key, "mykey");
    EXPECT_TRUE(out.value.empty());
}

TEST(ProtocolTest, ResponseRoundTrip) {
    Response in(Status::OK, "some data");
    auto bytes = ProtocolEncoder::encodeResponse(in);
    Response out = ProtocolEncoder::decodeResponse(bytes.data(), bytes.size());

    EXPECT_EQ(out.status, Status::OK);
    EXPECT_EQ(out.data, "some data");
}

TEST(ProtocolTest, ResponseNotFoundHasEmptyData) {
    Response in(Status::NOT_FOUND, "");
    auto bytes = ProtocolEncoder::encodeResponse(in);
    Response out = ProtocolEncoder::decodeResponse(bytes.data(), bytes.size());

    EXPECT_EQ(out.status, Status::NOT_FOUND);
    EXPECT_TRUE(out.data.empty());
}

TEST(ProtocolTest, EmptyKeyAndValueRoundTrip) {
    Request in(Command::DUMP, "", "");
    auto bytes = ProtocolEncoder::encodeRequest(in);
    Request out = ProtocolEncoder::decodeRequest(bytes.data(), bytes.size());

    EXPECT_EQ(out.cmd, Command::DUMP);
    EXPECT_TRUE(out.key.empty());
    EXPECT_TRUE(out.value.empty());
}

// A value near the uint16 length ceiling round-trips. This documents the
// current encoder's limit; Phase 2 widens the length fields to uint32.
TEST(ProtocolTest, LargeValueWithinUint16RoundTrips) {
    const std::string big(60000, 'x');
    Request in(Command::SET, "k", big);
    auto bytes = ProtocolEncoder::encodeRequest(in);
    Request out = ProtocolEncoder::decodeRequest(bytes.data(), bytes.size());

    EXPECT_EQ(out.value.size(), big.size());
    EXPECT_EQ(out.value, big);
}

TEST(ProtocolTest, ValueAboveUint16CeilingRoundTrips) {
    const std::string big(Protocol::MAX_VALUE_SIZE, 'y');
    ASSERT_GT(big.size(), 65535u);
    Request in(Command::SET, "k", big);
    auto bytes = ProtocolEncoder::encodeRequest(in);
    Request out = ProtocolEncoder::decodeRequest(bytes.data(), bytes.size());

    EXPECT_EQ(out.value.size(), big.size());
    EXPECT_EQ(out.value, big);
}
