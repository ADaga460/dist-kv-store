# Distributed Key-Value Store

A key-value store written from scratch in C++20: an asynchronous TCP server, an
ordered in-memory storage engine behind a swappable interface, a custom
length-prefixed binary protocol, and a command-line client.

The request model is being developed against a **bank scenario** (accounts,
transfers, per-account locks) purely as a concrete domain to drive the data model
and access patterns. It's a framing device, not the product.

## What's here today

Everything below is implemented and tested:

- **Async transport** (`src/net/`) — an ASIO `io_context` driven by a pool of
  threads accepts connections; each becomes a `Session` that reads and writes
  asynchronously. No thread is pinned to a connection.
- **Framing** — every message is a 4-byte big-endian length prefix followed by the
  payload, so a reader always assembles a complete message before decoding.
- **Binary protocol** (`src/protocol.cpp`) — a request is a command byte plus a
  `uint32`-length-prefixed key and value; a response is a status byte
  (`OK`/`NOT_FOUND`/`ERR`) plus `uint32`-length data. The decoder rejects malformed
  frames (returns `std::optional`) instead of silently misreading them.
- **Ordered storage engine** — `Store` (`src/store.cpp`) keeps data in a
  `std::map` behind a mutex and implements a `StorageEngine` interface
  (`get`/`set`/`scan`/`dump`/`size`). It supports point operations and **ordered
  prefix scans** (`scan("accounts/gov/")`), which a hash map can't do.
- **CLI client + server** speaking `SET`/`GET`/`DUMP` over the wire. (`scan` exists
  in the engine but isn't a wire command yet.)
- **Config** (`src/config.cpp`) — node id, host/port, workers, log level, etc. from
  CLI flags or a `--config` file; nothing is hardcoded.
- **Logging** via spdlog; **24 unit tests** via GoogleTest/CTest; a build + end-to-end
  test script (`run-tests.ps1`).

## Structure

The code is organized around clear seams so each layer can change independently:

```
Transport (net/)     TcpServer, Session, SyncClient — ASIO + framing
      │
Protocol             ProtocolEncoder — encode/decode requests & responses
      │
Storage              StorageEngine (interface)  ←— the swap point
                     └─ Store (ordered std::map + mutex)
```

The `StorageEngine` interface is the important boundary: callers depend on it, not
on `Store`, so a persistent or replicated backend can drop in later without
touching the server or protocol code.

```
include/       public headers
  net/         framing + connection (ASIO transport)
  storage_engine.h   the storage interface
src/           server, client, store, protocol, config
  net/         transport implementation
tests/         GoogleTest unit tests
run-tests.ps1  build + unit + end-to-end test script
CMakeLists.txt build + dependency fetching
```

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

In another terminal:

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

## Where this is going

The current single-node store is the foundation for a fault-tolerant, persistent
distributed store. The direction, roughly in order:

- **Persistence** — a write-ahead log + snapshots behind the same `StorageEngine`
  interface, so data survives restarts; eventually an on-disk B+tree replaces the
  in-memory map (the ordering contract stays identical, so callers don't change).
- **Replication** — Raft consensus across a cluster, turning the storage engine
  into a replicated state machine that stays consistent through node failures.
- **Coordination** — a distributed lock manager built on the ordered keyspace and
  consensus (leases + fencing tokens), which is what the bank scenario's contention
  is meant to exercise.
- **Domain layer** — fleshing out the bank scenario: accounts with `int64` cent
  balances (money is integer-only — no floats, ever), atomic transfers, and a
  double-entry ledger, as the concrete workload that stresses all of the above.

| Phase | What | Status |
|------:|------|--------|
| 0 | CMake, config, spdlog logging, tests | done |
| 1 | ASIO transport + framing, cross-platform | done |
| 2 | Ordered `StorageEngine`, `uint32` protocol lengths, decode-error handling, `scan` | done |
| 3 | Domain layer: accounts, integer-cent ledger, atomic transfers | next |
| 4 | Distributed lock manager — leases, fencing tokens, hierarchical locks | planned |
| 5 | Persistence: write-ahead log + snapshots | planned |
| 6 | Raft consensus — replication, leader redirect | planned |
| 7 | Cluster ops: dynamic membership, snapshot install | planned |
| 8 | Hardening: metrics, TLS, benchmarks, failure testing | planned |
