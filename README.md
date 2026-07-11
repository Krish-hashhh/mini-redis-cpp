# mini-redis-cpp

A multi-threaded, Redis-like in-memory cache server in C++20: TCP sockets,
thread-per-connection concurrency, LRU eviction, and disk persistence.

The whole thing is already built and working (see below). It's organized
as a **5-6 day reading/study plan** — each day points you at specific files
and concepts to actually understand, not just run. Don't skip to Day 5
without doing Day 1-4; each layer explains a piece you'll need for the next.

## Build & run

```bash
# cmake wasn't available when this was last built, so compile directly:
clang++ -std=c++20 -Wall -Wextra -Iinclude -pthread \
  -o build/mini_redis_server src/main_server.cpp src/lru_cache.cpp src/server.cpp

mkdir -p data
./build/mini_redis_server [port] [capacity]   # defaults: 7171, 100
```

Talk to it with `nc` (or any TCP client):

```bash
nc localhost 7171
SET foo bar
OK
GET foo
bar
SIZE
1
PING
PONG
QUIT
```

Press **Ctrl+C** to stop the server — it saves everything to `data/dump.txt`
first, and reloads it automatically on the next startup.

## The 5-6 day study plan

**Day 1 — The basic KV store**
Read `include/kv_store.hpp` + `src/kv_store.cpp` and run `./build/kv_cli`.
This is just a hash map wrapped in a class — no networking, no eviction,
no threads. Understand `std::unordered_map`, `std::optional` for "key not
found," and why this alone isn't good enough (unbounded memory growth,
one client only).

**Day 2 — LRU eviction**
Read `include/lru_cache.hpp` + `src/lru_cache.cpp`. This is the core data
structure: a doubly linked list (`std::list`) ordered by recency, plus a
hash map from key to a *list iterator* so both `get` and `set` are O(1).
Draw it on paper: insert a few keys, `get` one, insert past capacity, and
trace which node gets evicted (`order_.back()`) and why. This is the same
DS pattern as LeetCode's "LRU Cache" problem, just wired into a real
program.

**Day 3 — TCP sockets**
Read `runServer()` in `src/server.cpp` (bottom half of the file). Learn the
five-call skeleton every TCP server uses: `socket()` → `bind()` → `listen()`
→ `accept()` → `recv()`/`send()`. Run the server, connect with `nc`, and
watch `accept()` unblock each time you connect. Try starting two `nc`
sessions at once — see what threading in Day 5 is for.

**Day 4 — Protocol parsing & buffering**
Read `handleClient()` in `src/server.cpp`. The key insight: TCP is a byte
stream, not a message stream — one `recv()` might deliver half a command,
several commands, or exactly one. That's why there's a `buffer` string and
a loop that only processes text up to each `\n`. Try sending a command in
two separate `nc` keystrokes/pastes and confirm it still works.

**Day 5 — Concurrency (threads + mutex)**
Read the `std::thread(handleClient, ...).detach()` line in `runServer()`,
then go back to `lru_cache.hpp`/`.cpp` and look at `std::lock_guard<std::mutex>`
in every method. Understand: without that mutex, two clients writing at once
would corrupt the shared `std::list`/`unordered_map` (a data race). Test it
yourself — open two `nc` sessions, `SET` from both, confirm no crashes.

**Day 6 — Persistence**
Read `saveToDisk()` / `loadFromDisk()` and the `SIGINT` handler in
`src/main_server.cpp`. Note the comment about why the signal handler can't
directly call `saveToDisk()` (signal handlers can't safely lock a mutex or
do file I/O) — it just sets a flag that a small watcher thread polls.
Stop the server with Ctrl+C, inspect `data/dump.txt`, restart it, and
confirm your keys are back.

## Stretch goals (optional, once Day 1-6 feels solid)

These were part of the original 15-day plan — come back to them once
you're comfortable, not before:

- **TTL/expiry**: keys that auto-expire after N seconds
- **Thread pool**: replace thread-per-connection with a fixed worker pool
- **Lock sharding**: split the single mutex into N shards keyed by hash(key)
- **Append-only log**: log every write instead of snapshotting on exit
- **Benchmark client**: measure requests/sec under concurrent load

## Project layout

```
include/          public headers
  kv_store.hpp     Day 1 — plain hash-map store (used by kv_cli)
  lru_cache.hpp    Day 2 — LRU eviction, thread-safe (used by the server)
  server.hpp       Day 3-5 — TCP server interface
src/
  main_cli.cpp     Day 1 entry point (./build/kv_cli)
  main_server.cpp  Day 6 entry point (./build/mini_redis_server) — SIGINT save/load
  server.cpp       Day 3-5 — sockets, threading, protocol parsing
  lru_cache.cpp
  kv_store.cpp
data/              dump.txt (gitignored) — created on first SAVE/Ctrl+C
```
