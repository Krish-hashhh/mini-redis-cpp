#include "lru_cache.hpp"

void LRUCache::touch(std::list<Entry>::iterator it) {
    order_.splice(order_.begin(), order_, it); // move node to front in O(1)
}

void LRUCache::set(const std::string& key, const std::string& value) {
    std::lock_guard<std::mutex> lock(mutex_);

    auto found = index_.find(key);
    if (found != index_.end()) {
        found->second->second = value; // update value in place
        touch(found->second);
        return;
    }

    order_.emplace_front(key, value);
    index_[key] = order_.begin();

    if (index_.size() > capacity_) {
        // Evict least recently used = back of the list.
        auto& [evictKey, evictVal] = order_.back();
        index_.erase(evictKey);
        order_.pop_back();
    }
}

std::optional<std::string> LRUCache::get(const std::string& key) {
    std::lock_guard<std::mutex> lock(mutex_);

    auto found = index_.find(key);
    if (found == index_.end()) {
        return std::nullopt;
    }
    touch(found->second);
    return found->second->second;
}

bool LRUCache::del(const std::string& key) {
    std::lock_guard<std::mutex> lock(mutex_);

    auto found = index_.find(key);
    if (found == index_.end()) {
        return false;
    }
    order_.erase(found->second);
    index_.erase(found);
    return true;
}

size_t LRUCache::size() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return index_.size();
}

std::vector<std::pair<std::string, std::string>> LRUCache::snapshot() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<std::pair<std::string, std::string>> out(order_.rbegin(), order_.rend());
    return out;
}
