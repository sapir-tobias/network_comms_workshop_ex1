#pragma once

#include <arpa/inet.h>
#include <cerrno>
#include <chrono>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <limits>
#include <netdb.h>
#include <stdexcept>
#include <string>
#include <sys/socket.h>
#include <unistd.h>
#include <vector>

namespace bench {

constexpr uint16_t DEFAULT_PORT = 8080;
constexpr uint32_t MIN_MSG_SIZE = 1;                 // 1 B
constexpr uint32_t MAX_MSG_SIZE = 1u << 20;          // 1 MiB
constexpr uint32_t WARMUP_MESSAGES = 256;
constexpr uint64_t TARGET_BYTES_PER_ROUND = 256ull * 1024ull * 1024ull; // 256 MiB
constexpr uint64_t MIN_MESSAGES_PER_ROUND = 16;
constexpr uint64_t MAX_MESSAGES_PER_ROUND = 8ull * 1024ull * 1024ull;

struct RoundConfig {
    uint32_t msg_size;
    uint64_t warmup_messages;
    uint64_t measured_messages;
};

inline uint64_t calculate_messages(uint32_t msg_size, uint64_t target_bytes = TARGET_BYTES_PER_ROUND) {
    uint64_t x = target_bytes / msg_size;
    if (x < MIN_MESSAGES_PER_ROUND) x = MIN_MESSAGES_PER_ROUND;
    if (x > MAX_MESSAGES_PER_ROUND) x = MAX_MESSAGES_PER_ROUND;
    return x;
}

inline double now_seconds() {
    using clock = std::chrono::steady_clock;
    static const auto t0 = clock::now();
    const auto t = clock::now() - t0;
    return std::chrono::duration<double>(t).count();
}

inline void close_fd(int fd) {
    if (fd >= 0) close(fd);
}

inline void throw_errno(const std::string& what) {
    throw std::runtime_error(what + ": " + std::strerror(errno));
}

inline bool send_all(int fd, const void* data, size_t len) {
    const char* p = static_cast<const char*>(data);
    while (len > 0) {
        ssize_t n = send(fd, p, len, 0);
        if (n < 0) {
            if (errno == EINTR) continue;
            return false;
        }
        if (n == 0) return false;
        p += n;
        len -= static_cast<size_t>(n);
    }
    return true;
}

inline bool recv_all(int fd, void* data, size_t len) {
    char* p = static_cast<char*>(data);
    while (len > 0) {
        ssize_t n = recv(fd, p, len, MSG_WAITALL);
        if (n < 0) {
            if (errno == EINTR) continue;
            return false;
        }
        if (n == 0) return false;
        p += n;
        len -= static_cast<size_t>(n);
    }
    return true;
}

inline uint64_t htonll(uint64_t x) {
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
    return (static_cast<uint64_t>(htonl(static_cast<uint32_t>(x & 0xffffffffull))) << 32) |
           htonl(static_cast<uint32_t>(x >> 32));
#else
    return x;
#endif
}

inline uint64_t ntohll(uint64_t x) {
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
    return (static_cast<uint64_t>(ntohl(static_cast<uint32_t>(x & 0xffffffffull))) << 32) |
           ntohl(static_cast<uint32_t>(x >> 32));
#else
    return x;
#endif
}

inline bool send_u32(int fd, uint32_t x) {
    uint32_t net = htonl(x);
    return send_all(fd, &net, sizeof(net));
}

inline bool recv_u32(int fd, uint32_t& x) {
    uint32_t net = 0;
    if (!recv_all(fd, &net, sizeof(net))) return false;
    x = ntohl(net);
    return true;
}

inline bool send_u64(int fd, uint64_t x) {
    uint64_t net = htonll(x);
    return send_all(fd, &net, sizeof(net));
}

inline bool recv_u64(int fd, uint64_t& x) {
    uint64_t net = 0;
    if (!recv_all(fd, &net, sizeof(net))) return false;
    x = ntohll(net);
    return true;
}

inline bool send_round_config(int fd, const RoundConfig& cfg) {
    return send_u32(fd, cfg.msg_size) &&
           send_u64(fd, cfg.warmup_messages) &&
           send_u64(fd, cfg.measured_messages);
}

inline bool recv_round_config(int fd, RoundConfig& cfg) {
    return recv_u32(fd, cfg.msg_size) &&
           recv_u64(fd, cfg.warmup_messages) &&
           recv_u64(fd, cfg.measured_messages);
}

inline int connect_to_server(const std::string& host, uint16_t port) {
    addrinfo hints{};
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    addrinfo* result = nullptr;
    const std::string port_str = std::to_string(port);
    int rc = getaddrinfo(host.c_str(), port_str.c_str(), &hints, &result);
    if (rc != 0) {
        throw std::runtime_error(std::string("getaddrinfo failed: ") + gai_strerror(rc));
    }

    int fd = -1;
    for (addrinfo* p = result; p != nullptr; p = p->ai_next) {
        fd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if (fd < 0) continue;
        if (connect(fd, p->ai_addr, p->ai_addrlen) == 0) {
            freeaddrinfo(result);
            return fd;
        }
        close_fd(fd);
        fd = -1;
    }

    freeaddrinfo(result);
    throw_errno("connect failed");
    return -1;
}

inline int create_listening_socket(uint16_t port) {
    int fd = socket(AF_INET6, SOCK_STREAM, 0);
    if (fd < 0) throw_errno("socket failed");

    int opt = 1;
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    // Allow IPv4 clients too, when the OS supports dual-stack sockets.
    int off = 0;
    setsockopt(fd, IPPROTO_IPV6, IPV6_V6ONLY, &off, sizeof(off));

    sockaddr_in6 addr{};
    addr.sin6_family = AF_INET6;
    addr.sin6_addr = in6addr_any;
    addr.sin6_port = htons(port);

    if (bind(fd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0) {
        close_fd(fd);
        throw_errno("bind failed");
    }
    if (listen(fd, 1) < 0) {
        close_fd(fd);
        throw_errno("listen failed");
    }
    return fd;
}

inline bool drain_bytes(int fd, uint64_t total_bytes, std::vector<char>& buffer) {
    while (total_bytes > 0) {
        size_t want = buffer.size();
        if (total_bytes < want) want = static_cast<size_t>(total_bytes);

        ssize_t n = recv(fd, buffer.data(), want, 0);
        if (n < 0) {
            if (errno == EINTR) continue;
            return false;
        }
        if (n == 0) return false;
        total_bytes -= static_cast<uint64_t>(n);
    }
    return true;
}

inline void print_usage_client(const char* argv0) {
    std::cerr << "Usage: " << argv0 << " [server-host] [port] [target-MiB-per-size]\n"
              << "Example: " << argv0 << " 127.0.0.1 8080 256\n";
}

inline void print_usage_server(const char* argv0) {
    std::cerr << "Usage: " << argv0 << " [port]\n"
              << "Example: " << argv0 << " 8080\n";
}

} // namespace bench
