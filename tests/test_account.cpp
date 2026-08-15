#include <gtest/gtest.h>

#include <cstdint>

#include "account.h"
#include "store.h"

TEST(AccountTest, KeyFormat) {
    EXPECT_EQ(accountKey(AccountType::Gov, "treasury"), "accounts/gov/treasury");
    EXPECT_EQ(accountKey(AccountType::Personal, "alice"), "accounts/personal/alice");
}

TEST(AccountTest, SerializeRoundTrip) {
    Account in{AccountType::Corp, "Acme Inc", 5000000, AccountStatus::Frozen};
    auto out = deserializeAccount(serializeAccount(in));
    ASSERT_TRUE(out.has_value());
    EXPECT_EQ(out->type, AccountType::Corp);
    EXPECT_EQ(out->owner, "Acme Inc");
    EXPECT_EQ(out->balance_cents, 5000000);
    EXPECT_EQ(out->status, AccountStatus::Frozen);
}

TEST(AccountTest, BalanceHoldsInt64) {
    Account in{AccountType::Bank, "The Bank", 9000000000000000LL, AccountStatus::Open};
    auto out = deserializeAccount(serializeAccount(in));
    ASSERT_TRUE(out.has_value());
    EXPECT_EQ(out->balance_cents, 9000000000000000LL);
}

TEST(AccountTest, OwnerMayContainDelimiter) {
    Account in{AccountType::Personal, "a|b|c", 100, AccountStatus::Open};
    auto out = deserializeAccount(serializeAccount(in));
    ASSERT_TRUE(out.has_value());
    EXPECT_EQ(out->owner, "a|b|c");
    EXPECT_EQ(out->balance_cents, 100);
}

TEST(AccountTest, DeserializeGarbageReturnsNullopt) {
    EXPECT_FALSE(deserializeAccount("").has_value());
    EXPECT_FALSE(deserializeAccount("not-an-account").has_value());
    EXPECT_FALSE(deserializeAccount("gov|notanumber|open|x").has_value());
    EXPECT_FALSE(deserializeAccount("bogus|100|open|x").has_value());
}

TEST(AccountTest, DepositIncreasesBalance) {
    Store s;
    s.set("acct", serializeAccount({AccountType::Personal, "alice", 1000, AccountStatus::Open}));
    auto res = deposit(s, "acct", 500);
    EXPECT_EQ(res.status, TxStatus::Ok);
    EXPECT_EQ(res.balance, 1500);
    EXPECT_EQ(deserializeAccount(s.get("acct").second)->balance_cents, 1500);
}

TEST(AccountTest, WithdrawWithinBalance) {
    Store s;
    s.set("acct", serializeAccount({AccountType::Personal, "alice", 1000, AccountStatus::Open}));
    auto res = withdraw(s, "acct", 400);
    EXPECT_EQ(res.status, TxStatus::Ok);
    EXPECT_EQ(res.balance, 600);
}

TEST(AccountTest, WithdrawOverdraftRejectedAndBalanceUnchanged) {
    Store s;
    s.set("acct", serializeAccount({AccountType::Personal, "alice", 1000, AccountStatus::Open}));
    auto res = withdraw(s, "acct", 1500);
    EXPECT_EQ(res.status, TxStatus::InsufficientFunds);
    EXPECT_EQ(deserializeAccount(s.get("acct").second)->balance_cents, 1000);
}

TEST(AccountTest, TxOnMissingAccountIsNotFound) {
    Store s;
    EXPECT_EQ(deposit(s, "nope", 100).status, TxStatus::NotFound);
    EXPECT_EQ(withdraw(s, "nope", 100).status, TxStatus::NotFound);
}

TEST(AccountTest, NonPositiveAmountIsInvalid) {
    Store s;
    s.set("acct", serializeAccount({AccountType::Personal, "alice", 1000, AccountStatus::Open}));
    EXPECT_EQ(deposit(s, "acct", 0).status, TxStatus::InvalidAmount);
    EXPECT_EQ(withdraw(s, "acct", -5).status, TxStatus::InvalidAmount);
}

TEST(AccountTest, SeedPopulatesScannableAccounts) {
    Store s;
    seedAccounts(s);
    auto rows = s.scan("accounts/");
    EXPECT_EQ(rows.size(), 5u);

    auto alice = s.get("accounts/personal/alice");
    ASSERT_TRUE(alice.first);
    auto acct = deserializeAccount(alice.second);
    ASSERT_TRUE(acct.has_value());
    EXPECT_EQ(acct->balance_cents, 250000);
    EXPECT_EQ(acct->type, AccountType::Personal);
}
