#pragma once

#include <optional>
#include <string>
#include <unordered_map>

// Day 1: plain in-memory KV store, no thread-safety yet.
// Thread-safety (mutex/sharding) gets added starting Day 5/11 —
// don't add it now, it'll just be dead weight until there are threads.
class KVStore {
public:
    void set(const std::string& key, const std::string& value);
    std::optional<std::string> get(const std::string& key) const;
    bool del(const std::string& key);
    size_t size() const;

private:
    std::unordered_map<std::string, std::string> store_;
};
