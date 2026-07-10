cmake_minimum_required(VERSION 3.16)
project(mini_redis_cpp CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

add_compile_options(-Wall -Wextra -pthread)
link_libraries(pthread)

include_directories(${PROJECT_SOURCE_DIR}/include)

# Day 1: CLI-only KV store (no networking yet)
add_executable(kv_cli
  src/main_cli.cpp
  src/kv_store.cpp
)

# Server target — will grow as you add networking (Day 2+)
# Uncomment as you build src/server.cpp, src/protocol.cpp, etc.
# add_executable(mini_redis_server
#   src/main_server.cpp
#   src/kv_store.cpp
#   src/server.cpp
#   src/protocol.cpp
# )
