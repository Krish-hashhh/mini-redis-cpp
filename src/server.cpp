#include "server.hpp"

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <iostream>
#include <sstream>
#include <thread>

namespace {

// Executes one already-parsed line against the cache and returns the
// text response (without trailing newline — the caller adds that).
std::string handleCommand(const std::string& line, LRUCache& cache, bool& shouldClose) {
    std::istringstream iss(line);
    std::string cmd;
    iss >> cmd;

    if (cmd == "SET") {
        std::string key, value;
        iss >> key >> value;
        cache.set(key, value);
        return "OK";
    }
    if (cmd == "GET") {
        std::string key;
        iss >> key;
        auto val = cache.get(key);
        return val ? *val : "(nil)";
    }
    if (cmd == "DEL") {
        std::string key;
        iss >> key;
        return cache.del(key) ? "1" : "0";
    }
    if (cmd == "SIZE") {
        return std::to_string(cache.size());
    }
    if (cmd == "PING") {
        return "PONG";
    }
    if (cmd == "QUIT" || cmd == "EXIT") {
        shouldClose = true;
        return "BYE";
    }
    if (cmd.empty()) {
        return "";
    }
    return "ERR unknown command '" + cmd + "'";
}

// Reads from the socket and hands complete lines to handleCommand.
// TCP is a byte stream, not a message stream — a single recv() can
// contain half a command, multiple commands, or exactly one. We buffer
// bytes and only process text up to each '\n'.
void handleClient(int clientFd, LRUCache& cache) {
    std::string buffer;
    char chunk[4096];

    while (true) {
        ssize_t n = recv(clientFd, chunk, sizeof(chunk), 0);
        if (n <= 0) break; // 0 = client closed, <0 = error

        buffer.append(chunk, n);

        size_t newlinePos;
        while ((newlinePos = buffer.find('\n')) != std::string::npos) {
            std::string line = buffer.substr(0, newlinePos);
            if (!line.empty() && line.back() == '\r') line.pop_back(); // tolerate telnet \r\n
            buffer.erase(0, newlinePos + 1);

            bool shouldClose = false;
            std::string response = handleCommand(line, cache, shouldClose);
            if (!response.empty()) {
                response += "\n";
                send(clientFd, response.data(), response.size(), 0);
            }
            if (shouldClose) {
                close(clientFd);
                return;
            }
        }
    }

    close(clientFd);
}

} // namespace

void runServer(int port, LRUCache& cache) {
    int serverFd = socket(AF_INET, SOCK_STREAM, 0);
    if (serverFd < 0) {
        std::cerr << "socket() failed\n";
        return;
    }

    // Without this, restarting the server quickly after a crash fails with
    // "Address already in use" because the OS holds the port briefly.
    int opt = 1;
    setsockopt(serverFd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(static_cast<uint16_t>(port));

    if (bind(serverFd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0) {
        std::cerr << "bind() failed on port " << port << "\n";
        close(serverFd);
        return;
    }

    if (listen(serverFd, /*backlog=*/16) < 0) {
        std::cerr << "listen() failed\n";
        close(serverFd);
        return;
    }

    std::cout << "mini-redis-cpp listening on port " << port << "\n";

    while (true) {
        sockaddr_in clientAddr{};
        socklen_t clientLen = sizeof(clientAddr);
        int clientFd = accept(serverFd, reinterpret_cast<sockaddr*>(&clientAddr), &clientLen);
        if (clientFd < 0) continue; // e.g. interrupted by signal; keep serving

        // One thread per connection: simple to reason about, fine for a
        // learning project. A thread pool (fixed worker count) is the
        // natural upgrade once this makes sense — see README "next steps".
        std::thread(handleClient, clientFd, std::ref(cache)).detach();
    }
}
