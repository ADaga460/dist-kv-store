#include "account.h"

#include <array>
#include <stdexcept>

#include "storage_engine.h"

namespace {

std::string statusName(AccountStatus s) {
    switch (s) {
        case AccountStatus::Open: return "open";
        case AccountStatus::Frozen: return "frozen";
        case AccountStatus::Closed: return "closed";
    }
    return "open";
}

std::optional<AccountType> parseType(const std::string& s) {
    if (s == "gov") return AccountType::Gov;
    if (s == "corp") return AccountType::Corp;
    if (s == "personal") return AccountType::Personal;
    if (s == "bank") return AccountType::Bank;
    return std::nullopt;
}

std::optional<AccountStatus> parseStatus(const std::string& s) {
    if (s == "open") return AccountStatus::Open;
    if (s == "frozen") return AccountStatus::Frozen;
    if (s == "closed") return AccountStatus::Closed;
    return std::nullopt;
}

}  // namespace

std::string accountTypeName(AccountType type) {
    switch (type) {
        case AccountType::Gov: return "gov";
        case AccountType::Corp: return "corp";
        case AccountType::Personal: return "personal";
        case AccountType::Bank: return "bank";
    }
    return "personal";
}

std::string accountKey(AccountType type, const std::string& id) {
    return "accounts/" + accountTypeName(type) + "/" + id;
}

std::string serializeAccount(const Account& acct) {
    return accountTypeName(acct.type) + "|" + std::to_string(acct.balance_cents) + "|" +
           statusName(acct.status) + "|" + acct.owner;
}

std::optional<Account> deserializeAccount(const std::string& data) {
    const auto p1 = data.find('|');
    if (p1 == std::string::npos) return std::nullopt;
    const auto p2 = data.find('|', p1 + 1);
    if (p2 == std::string::npos) return std::nullopt;
    const auto p3 = data.find('|', p2 + 1);
    if (p3 == std::string::npos) return std::nullopt;

    auto type = parseType(data.substr(0, p1));
    auto status = parseStatus(data.substr(p2 + 1, p3 - p2 - 1));
    if (!type || !status) return std::nullopt;

    Account acct;
    acct.type = *type;
    acct.status = *status;
    acct.owner = data.substr(p3 + 1);
    try {
        acct.balance_cents = std::stoll(data.substr(p1 + 1, p2 - p1 - 1));
    } catch (const std::exception&) {
        return std::nullopt;
    }
    return acct;
}

void seedAccounts(StorageEngine& store) {
    store.set(accountKey(AccountType::Gov, "treasury"), serializeAccount({AccountType::Gov, "US Treasury", 100000000, AccountStatus::Open}));
    store.set(accountKey(AccountType::Corp, "acme"), serializeAccount({AccountType::Corp, "Acme Inc", 5000000, AccountStatus::Open}));
    store.set(accountKey(AccountType::Personal, "alice"), serializeAccount({AccountType::Personal, "Alice", 250000, AccountStatus::Open}));
    store.set(accountKey(AccountType::Personal, "bob"), serializeAccount({AccountType::Personal, "Bob", 100000, AccountStatus::Open}));
    store.set(accountKey(AccountType::Bank, "reserve"), serializeAccount({AccountType::Bank, "The Bank", 1000000000, AccountStatus::Open}));
}
