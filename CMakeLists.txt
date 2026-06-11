cmake_minimum_required(VERSION 3.15)
project(TcpThroughputBenchmark CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(server benchmark_server.cpp)
add_executable(client benchmark_client.cpp)

target_compile_options(server PRIVATE -O2 -Wall -Wextra -pedantic)
target_compile_options(client PRIVATE -O2 -Wall -Wextra -pedantic)