#pragma once
#include <chrono>
#include <sys/socket.h>
#include <unistd.h>


// Basic network configuration
constexpr int PORT = 8080;
const char* const HOST = "127.0.0.1"; // Localhost

// Benchmark parameters
constexpr int MIN_EXPONENT = 0;   // 2^0 = 1 Byte
constexpr int MAX_EXPONENT = 20;  // 2^20 = 1 Megabyte

constexpr size_t NUM_WARMUP_MESSAGES = 100;

// The target data volume we want to send per size step to get a good reading
constexpr size_t TARGET_DATA_VOLUME = 16 * 1024 * 1024; // 16 MB

// Simple function to calculate how many messages (X) to send
inline size_t calculate_X(int msg_size) {
    size_t X = TARGET_DATA_VOLUME / msg_size;
    if (X < 10) X = 10; // Send at least 10 messages for large sizes
    return X;
}