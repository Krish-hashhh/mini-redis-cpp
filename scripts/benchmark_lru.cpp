// Benchmark: this project's O(1) LRUCache (hashmap + doubly-linked-list splice)
// vs. a naive O(n) LRU cache (vector, linear scan to find/evict) — both
// guarded by a single std::mutex, run under concurrent multi-threaded load.
//
// Build (from project root):
//   clang++ -std=c++20 -O2 -Wall -Wextra -Iinclude -pthread \
//       scripts/benchmark_lru.cpp src/lru_cache.cpp -o build/lru_benchmark
// Run:
//   ./build/lru_benchmark

#include "lru_cache.hpp"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstdio>
#include <mutex>
#include <numeric>
#include <random>
#include <string>
#include <thread>
#include <vector>

// --- Naive baseline: vector-backed LRU, O(n) find + O(n) evict, single mutex ---
class NaiveLRUCache {
public:
    explicit NaiveLRUCache(size_t capacity) : capacity_(capacity) {}

    void set(const std::string& key, const std::string& value) {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = std::find_if(entries_.begin(), entries_.end(),
                                [&](auto& e) { return e.first == key; });
        if (it != entries_.end()) {
            it->second = value;
            std::rotate(entries_.begin(), it, it + 1); // move to front
            return;
        }
        entries_.insert(entries_.begin(), {key, value});
        if (entries_.size() > capacity_) entries_.pop_back();
    }

    bool get(const std::string& key, std::string& out) {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = std::find_if(entries_.begin(), entries_.end(),
                                [&](auto& e) { return e.first == key; });
        if (it == entries_.end()) return false;
        out = it->second;
        std::rotate(entries_.begin(), it, it + 1);
        return true;
    }

private:
    size_t capacity_;
    std::mutex mutex_;
    std::vector<std::pair<std::string, std::string>> entries_; // front = MRU
};

struct Result {
    double ops_per_sec;
    double avg_latency_ns;
};

template <typename CacheT, typename SetFn, typename GetFn>
Result run_bench(size_t num_threads, size_t ops_per_thread, size_t key_space,
                  SetFn set_op, GetFn get_op) {
    std::atomic<size_t> total_ops{0};
    auto start = std::chrono::steady_clock::now();

    std::vector<std::thread> threads;
    for (size_t t = 0; t < num_threads; ++t) {
        threads.emplace_back([&, t] {
            std::mt19937 rng(static_cast<unsigned>(t) + 1);
            std::uniform_int_distribution<size_t> key_dist(0, key_space - 1);
            std::uniform_int_distribution<int> op_dist(0, 4); // 20% set, 80% get

            for (size_t i = 0; i < ops_per_thread; ++i) {
                std::string key = "key:" + std::to_string(key_dist(rng));
                if (op_dist(rng) == 0) {
                    set_op(key, "value:" + key);
                } else {
                    get_op(key);
                }
            }
            total_ops.fetch_add(ops_per_thread, std::memory_order_relaxed);
        });
    }
    for (auto& th : threads) th.join();

    auto end = std::chrono::steady_clock::now();
    double seconds = std::chrono::duration<double>(end - start).count();
    double ops = static_cast<double>(total_ops.load());

    return {ops / seconds, (seconds * 1e9) / ops};
}

int main() {
    constexpr size_t kThreads = 8;
    constexpr size_t kOpsPerThread = 100'000;
    constexpr size_t kCapacity = 1000;
    constexpr size_t kKeySpace = 5000; // > capacity, forces real eviction traffic
    constexpr int kRuns = 5;

    std::vector<double> lru_throughput, lru_latency, naive_throughput, naive_latency;

    for (int run = 0; run < kRuns; ++run) {
        {
            LRUCache cache(kCapacity);
            auto r = run_bench<LRUCache>(
                kThreads, kOpsPerThread, kKeySpace,
                [&](const std::string& k, const std::string& v) { cache.set(k, v); },
                [&](const std::string& k) { cache.get(k); });
            lru_throughput.push_back(r.ops_per_sec);
            lru_latency.push_back(r.avg_latency_ns);
        }
        {
            NaiveLRUCache cache(kCapacity);
            auto r = run_bench<NaiveLRUCache>(
                kThreads, kOpsPerThread, kKeySpace,
                [&](const std::string& k, const std::string& v) { cache.set(k, v); },
                [&](const std::string& k) { std::string out; cache.get(k, out); });
            naive_throughput.push_back(r.ops_per_sec);
            naive_latency.push_back(r.avg_latency_ns);
        }
        std::printf("run %d done\n", run + 1);
    }

    auto avg = [](const std::vector<double>& v) {
        return std::accumulate(v.begin(), v.end(), 0.0) / v.size();
    };

    double lru_ops = avg(lru_throughput), lru_ns = avg(lru_latency);
    double naive_ops = avg(naive_throughput), naive_ns = avg(naive_latency);

    std::printf("\n=== Results (avg over %d runs, %zu threads, %zu ops/thread) ===\n",
                kRuns, kThreads, kOpsPerThread);
    std::printf("O(1) LRUCache   : %.0f ops/sec, %.1f ns/op\n", lru_ops, lru_ns);
    std::printf("Naive O(n) LRU  : %.0f ops/sec, %.1f ns/op\n", naive_ops, naive_ns);
    std::printf("Speedup         : %.2fx throughput, %.2fx lower latency\n",
                lru_ops / naive_ops, naive_ns / lru_ns);

    return 0;
}
