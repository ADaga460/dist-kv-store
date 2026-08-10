# Distributed Bank Simulator

A faux bank you can hammer from many machines at once. The **server is the bank**;
**clients are nodes** — individuals moving their own money, and systems (payroll,
tax, settlement) sweeping many accounts. The bank holds **government, corporate,
personal-savings, and its own** accounts, each with a lock.

The whole point is **contention**: lots of nodes racing to move the same money.
Distributed locks with leases and fencing tokens keep it correct — no double-spend,
no lost updates, money always conserved. Under the hood it's a strongly-consistent
distributed key-value store (Raft-replicated), so the bank survives node failures.

> **Money is integer cents (`int64`) only.** No floats, no decimals, no string
> amounts — anywhere. Every amount on the wire, in the store, and in the ledger is
> an integer, and non-integer amounts are rejected. Floating-point money loses
> pennies; this project refuses to.

## Status

Early. The distributed KV **foundation** is built and working; the bank layer is
next. Concretely:

- **Done:** cross-platform build, async ASIO networking with proper framing,
  config, logging, tests, and a working `SET`/`GET`/`DUMP` store.
- **Next:** ordered store → accounts + integer-cent ledger → atomic transfers →
  the distributed lock manager → persistence → Raft. See the [roadmap](#roadmap).

## Build

Needs a C++20 compiler (e.g. `g++`) and CMake. CMake fetches the dependencies
(ASIO, spdlog, GoogleTest) on first configure.

```
cmake -S . -B build -G Ninja
cmake --build build
```

Produces `server` and `client` in `build/`.

## Run

```
build/server --listen_port 8080 --log_level info
```

In another terminal (today's commands — bank ops land with Phase 3):

```
build/client set user1 Alice
build/client get user1
build/client dump
```

Flags override any default, or point at a file with `--config <file>`.

## Test

```
ctest --test-dir build --output-on-failure
```

Or the all-in-one script (build + unit tests + end-to-end):

```
./run-tests.ps1
```

## How it works (today)

**Framing.** Each message is a 4-byte big-endian length prefix followed by the
payload, so the reader always pulls a full message before decoding.

**Protocol.** Inside a frame: a request is a command byte + key + value (each
length-prefixed); a response is a status byte (`OK`/`NOT_FOUND`/`ERR`) + data.

**Server.** ASIO accepts connections; each becomes an async session that reads a
framed request, runs it against the store, and writes a framed response. No thread
is pinned to a connection.

**Store.** An in-memory map behind a mutex. This becomes an **ordered** store in
Phase 2 — the bank needs ordered keys for deadlock-safe lock ordering, range/prefix
locks, category scans, and fair wait-queues.

## The bank model (planned)

Namespaced keyspace:

```
/accounts/gov/<id>        balance in int64 cents, owner, status
/accounts/corp/<id>
/accounts/personal/<id>
/accounts/bank/<id>       the bank's own reserves
/ledger/<acct>/<ts>-<seq> append-only double-entry history
/locks/accounts/<id>      holder, lease, fencing token
```

The core operation is `TRANSFER(from, to, amount, txn_id)`: lock both accounts in
ascending id order (so opposing transfers can't deadlock), check the balance,
debit + credit with matching ledger entries, commit, release. A repeated `txn_id`
is applied at most once.

## Roadmap

| Phase | What |
|------:|------|
| 0 ✅ | CMake, config, spdlog logging, tests |
| 1 ✅ | ASIO transport + framing, cross-platform (retire Winsock2) |
| 2 | Ordered `StorageEngine` (`std::map`), protocol v2 (`uint32` lengths), `SCAN` |
| 3 | Bank domain: accounts, integer-cent double-entry ledger, atomic transfers |
| 4 | **Distributed lock manager** — leases, fencing tokens, hierarchical locks |
| 5 | Persistence: write-ahead log + snapshots |
| 6 | Raft consensus — replication, leader redirect, fencing = log index |
| 7 | Cluster ops: dynamic membership, snapshot install |
| 8 | Hardening: contention demo, metrics, TLS, benchmarks, failure testing |

## Layout

```
include/       public headers
  net/         framing + connection (ASIO transport)
src/           server, client, store, protocol, config
  net/         transport implementation
tests/         GoogleTest unit tests
run-tests.ps1  build + unit + end-to-end test script
CMakeLists.txt build + dependency fetching
```
