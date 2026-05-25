cmake_minimum_required(VERSION 3.15)
project(NetworkTesting CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Compile optimizations to make future throughput calculations hyper-fast
add_compile_options(-O3)

# Define the Server Executable (Compiles tcp_server.cpp into an executable named 'server')
add_executable(server tcp_server.cpp shared.h)

# Define the Client Executable (Compiles tcp_client.cpp into an executable named 'client')
add_executable(client tcp_client.cpp shared.h)