# Distributed KV Store

An in-memory key-value store written in C++. A TCP server holds the data, and a small command-line client talks to it over a custom binary protocol. Runs on Windows using Winsock2.

The server uses a thread pool so multiple clients can hit it at the same time, and a mutex keeps the store safe under concurrent access.

## Commands

- `set <key> <value>` — store a value
- `get <key>` — read a value back
- `dump` — list everything in the store

## Build

You need `g++` (MinGW) on your PATH.

```
build.bat
```

This drops `server.exe` and `client.exe` into `build/`.

## Run

Start the server in one terminal:

```
build\server.exe
```

It listens on port `8080` with 4 worker threads.

Then use the client from another terminal:

```
build\client.exe set name aarav
build\client.exe get name
build\client.exe dump
```

The client connects to `127.0.0.1:8080`.

## How it works

**Protocol.** Every message is length-prefixed binary, not text. A request is one command byte, then the key (2-byte length + bytes), then the value (2-byte length + bytes). A response is a status byte (`OK`, `NOT_FOUND`, or `ERR`) followed by a length-prefixed data blob. Lengths are little-endian `uint16`.

**Server.** `accept()` runs on the main thread. Each new connection gets handed to the thread pool, and a worker reads requests in a loop until the client disconnects.

**Store.** Just an `unordered_map` behind a mutex. Every read and write takes the lock, so the map stays consistent no matter how many workers are active.

## Layout

```
include/   headers (protocol, store, threadpool)
src/       server, client, and the pieces above
build.bat  compiles everything
```

## Limits

- Keys up to 256 bytes, values up to 64KB
- Data lives in memory only — restart the server and it's gone
- Windows only (depends on Winsock2)
