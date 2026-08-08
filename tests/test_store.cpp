#include <gtest/gtest.h>

#include <string>
#include <thread>
#include <vector>

#include "store.h"

TEST(StoreTest, SetThenGetReturnsValue) {
    Store s;
    s.set("k", "v");
    auto [found, value] = s.get("k");
    EXPECT_TRUE(found);
    EXPECT_EQ(value, "v");
}

TEST(StoreTest, GetMissingKeyReportsNotFound) {
    Store s;
    auto [found, value] = s.get("nope");
    EXPECT_FALSE(found);
    EXPECT_TRUE(value.empty());
}

TEST(StoreTest, SetOverwritesExistingValue) {
    Store s;
    s.set("k", "first");
    s.set("k", "second");
    auto [found, value] = s.get("k");
    EXPECT_TRUE(found);
    EXPECT_EQ(value, "second");
}

TEST(StoreTest, SizeReflectsDistinctKeys) {
    Store s;
    EXPECT_EQ(s.size(), 0u);
    s.set("a", "1");
    s.set("b", "2");
    s.set("a", "3");  // overwrite, not a new key
    EXPECT_EQ(s.size(), 2u);
}

TEST(StoreTest, DumpContainsKeysAndCount) {
    Store s;
    s.set("alpha", "1");
    s.set("beta", "2");
    const std::string dump = s.dump();
    EXPECT_NE(dump.find("Total keys: 2"), std::string::npos);
    EXPECT_NE(dump.find("alpha = 1"), std::string::npos);
    EXPECT_NE(dump.find("beta = 2"), std::string::npos);
}

// The store guards its map with a mutex; concurrent writers to distinct keys
// must all land without corrupting the map.
TEST(StoreTest, ConcurrentWritesAreSafe) {
    Store s;
    constexpr int kThreads = 8;
    constexpr int kPerThread = 500;

    std::vector<std::thread> threads;
    for (int t = 0; t < kThreads; ++t) {
        threads.emplace_back([&s, t] {
            for (int i = 0; i < kPerThread; ++i) {
                s.set("k" + std::to_string(t) + "_" + std::to_string(i), "v");
            }
        });
    }
    for (auto& th : threads) th.join();

    EXPECT_EQ(s.size(), static_cast<size_t>(kThreads * kPerThread));
}
