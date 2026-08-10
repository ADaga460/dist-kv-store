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

TEST(StoreTest, ScanPrefixReturnsMatchingKeysSorted) {
    Store s;
    s.set("accounts/gov/2", "b");
    s.set("accounts/gov/1", "a");
    s.set("accounts/corp/1", "c");
    auto rows = s.scan("accounts/gov/");
    ASSERT_EQ(rows.size(), 2u);
    EXPECT_EQ(rows[0].first, "accounts/gov/1");
    EXPECT_EQ(rows[0].second, "a");
    EXPECT_EQ(rows[1].first, "accounts/gov/2");
    EXPECT_EQ(rows[1].second, "b");
}

TEST(StoreTest, ScanEmptyPrefixReturnsAllSorted) {
    Store s;
    s.set("b", "2");
    s.set("a", "1");
    s.set("c", "3");
    auto rows = s.scan("");
    ASSERT_EQ(rows.size(), 3u);
    EXPECT_EQ(rows[0].first, "a");
    EXPECT_EQ(rows[1].first, "b");
    EXPECT_EQ(rows[2].first, "c");
}

TEST(StoreTest, ScanNonMatchingPrefixReturnsEmpty) {
    Store s;
    s.set("apple", "1");
    s.set("banana", "2");
    EXPECT_TRUE(s.scan("cherry").empty());
}

TEST(StoreTest, ScanPrefixBoundary) {
    Store s;
    s.set("acc", "x");
    s.set("acct/1", "1");
    s.set("acctx", "2");
    s.set("bcct", "y");
    auto rows = s.scan("acct");
    ASSERT_EQ(rows.size(), 2u);
    EXPECT_EQ(rows[0].first, "acct/1");
    EXPECT_EQ(rows[1].first, "acctx");
}
