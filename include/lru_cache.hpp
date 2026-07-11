#pragma once

#include <list>
#include <mutex>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

// A fixed-capacity, thread-safe LRU (Least Recently Used) cache.
//
// How it works:
//   - `order_` is a doubly linked list of {key, value} pairs, kept in
//     recency order: front = most recently used, back = least recently used.
//   - `index_` maps key -> iterator into `order_`, so we can jump straight
//     to a node instead of scanning the list (that's what makes get/set O(1)).
//   - On every get/set of an existing key, we splice that node to the front.
//   - On set of a new key past capacity, we evict the back of the list.
//
// Thread-safety: one mutex guards both containers. It's coarse-grained
// (the whole cache locks per operation) — simple and correct, though not
// the fastest possible design. Lock sharding is the natural next upgrade
// once you're comfortable with this version.
class LRUCache {
public:
    explicit LRUCache(size_t capacity) : capacity_(capacity) {}

    void set(const std::string& key, const std::string& value);
    std::optional<std::string> get(const std::string& key);
    bool del(const std::string& key);
    size_t size() const;

    // Returns all entries, oldest-first, for persistence (SAVE).
    std::vector<std::pair<std::string, std::string>> snapshot() const;

private:
    using Entry = std::pair<std::string, std::string>; // {key, value}

    size_t capacity_;
    mutable std::mutex mutex_;
    std::list<Entry> order_;                                         // front = MRU
    std::unordered_map<std::string, std::list<Entry>::iterator> index_;

    void touch(std::list<Entry>::iterator it); // move node to front, caller holds lock
};
