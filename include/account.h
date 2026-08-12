#pragma once
#include <cstdint>
#include <optional>
#include <string>

class StorageEngine;

enum class AccountType { Gov, Corp, Personal, Bank };
enum class AccountStatus { Open, Frozen, Closed };

struct Account {
    AccountType type = AccountType::Personal;
    std::string owner;
    int64_t balance_cents = 0;
    AccountStatus status = AccountStatus::Open;
};

std::string accountTypeName(AccountType type);
std::string accountKey(AccountType type, const std::string& id);

std::string serializeAccount(const Account& acct);
std::optional<Account> deserializeAccount(const std::string& data);

void seedAccounts(StorageEngine& store);
