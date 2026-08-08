#include <gtest/gtest.h>

#include <vector>

#include "config.h"

namespace {
// Helper: build an argv-style array from a vector of strings.
bool parse(Config& c, std::vector<std::string> args) {
    std::vector<char*> argv;
    argv.push_back(const_cast<char*>("client"));
    for (auto& a : args) argv.push_back(const_cast<char*>(a.c_str()));
    return c.parseArgs(static_cast<int>(argv.size()), argv.data());
}
}  // namespace

TEST(ConfigTest, DefaultsAreSane) {
    Config c;
    EXPECT_EQ(c.listen_port, 8080);
    EXPECT_EQ(c.worker_threads, 4u);
    EXPECT_EQ(c.connect_host, "127.0.0.1");
}

TEST(ConfigTest, SpaceSeparatedFlagsOverrideDefaults) {
    Config c;
    ASSERT_TRUE(parse(c, {"--listen_port", "9000", "--worker_threads", "8"}));
    EXPECT_EQ(c.listen_port, 9000);
    EXPECT_EQ(c.worker_threads, 8u);
}

TEST(ConfigTest, EqualsFormFlags) {
    Config c;
    ASSERT_TRUE(parse(c, {"--node_id=node-7", "--listen_host=0.0.0.0"}));
    EXPECT_EQ(c.node_id, "node-7");
    EXPECT_EQ(c.listen_host, "0.0.0.0");
}

TEST(ConfigTest, PeersParseAsCsv) {
    Config c;
    ASSERT_TRUE(parse(c, {"--peers", "a:1,b:2,c:3"}));
    ASSERT_EQ(c.peers.size(), 3u);
    EXPECT_EQ(c.peers[0], "a:1");
    EXPECT_EQ(c.peers[2], "c:3");
}

TEST(ConfigTest, MissingValueForFlagFails) {
    Config c;
    EXPECT_FALSE(parse(c, {"--listen_port"}));
}
