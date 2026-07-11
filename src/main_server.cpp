#include <chrono>
#include <csignal>
#include <fstream>
#include <iostream>
#include <string>
#include <thread>

#include "lru_cache.hpp"
#include "server.hpp"

namespace {

constexpr const char* kDumpFile = "data/dump.txt";

// Very small persistence format: one "key value" pair per line, oldest
// (least recently used) first. Good enough to survive a restart; not a
// real append-only log (that's a later, harder upgrade).
void saveToDisk(const LRUCache& cache) {
    std::ofstream out(kDumpFile, std::ios::trunc);
    if (!out) {
        std::cerr << "warning: could not write " << kDumpFile << "\n";
        return;
    }
    for (const auto& [key, value] : cache.snapshot()) {
        out << key << ' ' << value << '\n';
    }
    std::cout << "saved " << cache.size() << " keys to " << kDumpFile << "\n";
}

void loadFromDisk(LRUCache& cache) {
    std::ifstream in(kDumpFile);
    if (!in) return; // no dump yet, e.g. first run

    std::string key, value;
    size_t loaded = 0;
    while (in >> key >> value) {
        cache.set(key, value);
        ++loaded;
    }
    std::cout << "loaded " << loaded << " keys from " << kDumpFile << "\n";
}

// The signal handler can't safely touch LRUCache (mutex, I/O aren't
// signal-safe), so it only sets a flag; a tiny watcher thread does the
// actual save. This is the standard pattern for "do real work on Ctrl+C."
volatile std::sig_atomic_t g_shutdownRequested = 0;

void handleSigint(int) {
    g_shutdownRequested = 1;
}

} // namespace

int main(int argc, char** argv) {
    int port = 7171;
    size_t capacity = 100;

    if (argc >= 2) port = std::stoi(argv[1]);
    if (argc >= 3) capacity = static_cast<size_t>(std::stoul(argv[2]));

    LRUCache cache(capacity);
    loadFromDisk(cache);

    std::signal(SIGINT, handleSigint);

    std::thread shutdownWatcher([&cache]() {
        while (!g_shutdownRequested) {
            std::this_thread::sleep_for(std::chrono::milliseconds(200));
        }
        std::cout << "\nCtrl+C received, saving before exit...\n";
        saveToDisk(cache);
        std::cout.flush();
        std::exit(0); // flushes stdio buffers and runs atexit handlers, unlike _Exit
    });
    shutdownWatcher.detach();

    std::cout << "capacity=" << capacity << " (LRU eviction once full)\n";
    runServer(port, cache); // blocks forever; Ctrl+C exits via the watcher above

    return 0;
}
