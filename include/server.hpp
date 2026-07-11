#pragma once

#include <string>

#include "lru_cache.hpp"

// Runs a blocking TCP server on the given port: accepts connections in a
// loop and spawns a detached thread per client. Each client thread parses
// newline-terminated text commands and executes them against `cache`.
//
// Protocol (one command per line, response ends with \n):
//   SET key value   -> OK
//   GET key         -> value | (nil)
//   DEL key         -> 1 | 0
//   SIZE            -> <count>
//   PING            -> PONG
//   QUIT            -> closes the connection
//
// Call stops only on process exit (Ctrl+C) — see main_server.cpp for the
// SIGINT handler that saves to disk before exiting.
void runServer(int port, LRUCache& cache);
