#include <iostream>
#include <sstream>
#include <string>

#include "kv_store.hpp"

// Day 1 milestone: a REPL that talks to KVStore directly, no sockets yet.
// Try: SET foo bar / GET foo / DEL foo / SIZE / QUIT
int main() {
    KVStore store;
    std::string line;

    std::cout << "mini-redis-cpp (Day 1 CLI) — type QUIT to exit\n";

    while (true) {
        std::cout << "> ";
        if (!std::getline(std::cin, line)) break;
        if (line.empty()) continue;

        std::istringstream iss(line);
        std::string cmd;
        iss >> cmd;

        if (cmd == "SET") {
            std::string key, value;
            iss >> key >> value;
            store.set(key, value);
            std::cout << "OK\n";
        } else if (cmd == "GET") {
            std::string key;
            iss >> key;
            auto val = store.get(key);
            std::cout << (val ? *val : "(nil)") << "\n";
        } else if (cmd == "DEL") {
            std::string key;
            iss >> key;
            std::cout << (store.del(key) ? "1" : "0") << "\n";
        } else if (cmd == "SIZE") {
            std::cout << store.size() << "\n";
        } else if (cmd == "QUIT" || cmd == "EXIT") {
            break;
        } else {
            std::cout << "ERR unknown command '" << cmd << "'\n";
        }
    }

    return 0;
}
