# mini-redis-cpp

A multi-threaded, Redis-like in-memory cache server built in C++20 as a 15-day
learning project — networking, concurrency, custom protocols, eviction
policies, and persistence.

## Roadmap

| Day | Milestone |
|-----|-----------|
| 1   | Core KV store + CLI REPL (no networking) |
| 2   | Single-client TCP server skeleton |
| 3   | Wire KV store into the socket loop |
| 4   | Real length-prefixed protocol parser |
| 5   | Multi-client via thread-per-connection |
| 6   | Graceful shutdown / disconnect handling |
| 7   | LRU eviction (hashmap + doubly linked list) |
| 8   | TTL / expiry (passive + active sweep) |
| 9   | Stress test + race condition fixes |
| 10  | Thread pool (replace thread-per-connection) |
| 11  | Lock sharding / `shared_mutex` for reads |
| 12  | Snapshot persistence (SAVE / load on boot) |
| 13  | Append-only log (AOF) |
| 14  | Benchmark client + `INFO` stats |
| 15  | Cleanup, README, stretch goal (pub/sub, config file, or Docker) |

## Build

```bash
mkdir -p build && cd build
cmake ..
make -j
```

## Run (Day 1)

```bash
./build/kv_cli
> SET foo bar
OK
> GET foo
bar
> SIZE
1
> QUIT
```

## Project layout

```
include/    public headers (kv_store.hpp, protocol.hpp, ...)
src/        implementation files
tests/      test client scripts / unit tests
scripts/    benchmark and helper scripts
data/       snapshot/AOF files (gitignored)
```
