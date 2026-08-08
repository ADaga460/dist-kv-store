# Distributed KV Store

An in-memory key-value store written in C++. A TCP server holds the data, and a
small command-line client talks to it over a custom binary protocol. Networking
runs on standalone [ASIO](https://think-async.com/Asio/), so it builds and runs
on both Windows and Linux.

The server is async — a single `io_context` driven by a pool of threads handles
all connections — and a mutex keeps the store consistent under concurrent access.

> **Where this is heading:** right now this is a solid single node. The
> [roadmap](#roadmap) turns it into a real distributed store (Raft replication,
> persistence, clustering). Today's code is Phase 0–1: build system, config,
> logging, tests, and the ASIO transport.

## Commands

- `set <key> <value>` — store a value
- `get <key>` — read a value back
- `dump` — list everything in the store

## Build

You need `g++` (or any C++20 compiler) and CMake. CMake pulls the dependencies
(ASIO, spdlog, GoogleTest) automatically on first configure.

```
cmake -S . -B build -G Ninja
cmake --build build
```

This produces `server` and `client` in `build/`.

## Run

Start the server:

```
build/server
```

It listens on `0.0.0.0:8080` with 4 worker threads by default. Override anything
with flags (or a `--config <file>`):

```
build/server --listen_port 9000 --worker_threads 8 --log_level debug
```

Then use the client from another terminal:

```
build/client set name aarav
build/client get name
build/client dump
build/client --connect_host 127.0.0.1 --connect_port 9000 get name
```

## Test

```
ctest --test-dir build --output-on-failure
```

## How it works

**Framing.** Each message on the wire is a 4-byte big-endian length prefix
followed by the payload. The reader pulls a full frame before decoding, so it
never assumes one `recv()` equals one message.

**Protocol.** Inside a frame, a request is one command byte, then the key
(length + bytes), then the value (length + bytes). A response is a status byte
(`OK`, `NOT_FOUND`, or `ERR`) followed by a length-prefixed data blob.

**Server.** ASIO accepts connections; each becomes a `Session` that asynchronously
reads a framed request, runs it against the store, and writes a framed response.
No thread is pinned to a connection.

**Store.** An `unordered_map` behind a mutex. Every read and write takes the lock,
so the map stays consistent no matter how many workers are active.

## Layout

```
include/       public headers
  net/         framing + connection (ASIO transport)
src/           server, client, store, protocol, config
  net/         transport implementation
tests/         GoogleTest unit tests
CMakeLists.txt build + dependency fetching
```

## Roadmap

The full plan lives in the approved roadmap, but the short version:

| Phase | What |
|------:|------|
| 0 ✅ | CMake, config, spdlog logging, tests |
| 1 ✅ | ASIO transport, proper framing, cross-platform (retire Winsock2) |
| 2 | Protocol v2 (`uint32` lengths, `DELETE`/`CAS`), `StorageEngine` interface |
| 3 | Persistence: write-ahead log + snapshots |
| 4 | Raft consensus — leader election, log replication, linearizable reads |
| 5 | Cluster ops: dynamic membership, snapshot install |
| 6 | Hardening: metrics, TLS, benchmarks, failure testing |

## Limits (today)

- Keys up to 256 bytes, values up to 64KB
- Data lives in memory only — restart the server and it's gone (Phase 3 fixes this)
- Single node — not yet replicated (Phase 4 fixes this)
