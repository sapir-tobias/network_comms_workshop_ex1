#include "benchmark_shared.hpp"
#include <iomanip>

int main(int argc, char* argv[]) {
    try {
        if (argc > 4) {
            bench::print_usage_client(argv[0]);
            return 1;
        }

        std::string host = (argc >= 2) ? argv[1] : "127.0.0.1";
        uint16_t port = (argc >= 3) ? static_cast<uint16_t>(std::stoi(argv[2])) : bench::DEFAULT_PORT;
        uint64_t target_mib = (argc >= 4) ? static_cast<uint64_t>(std::stoull(argv[3])) : (bench::TARGET_BYTES_PER_ROUND / (1024ull * 1024ull));
        uint64_t target_bytes = target_mib * 1024ull * 1024ull;

        int sock = bench::connect_to_server(host, port);
        std::cerr << "[+] Connected to " << host << ":" << port << "\n";

        std::vector<char> buffer(bench::MAX_MSG_SIZE, 'A');

        std::cout << std::left
                  << std::setw(14) << "size_bytes"
                  << std::setw(14) << "messages"
                  << std::setw(16) << "MiB_sent"
                  << std::setw(14) << "seconds"
                  << std::setw(14) << "Mbps" << "\n";

        for (uint32_t msg_size = bench::MIN_MSG_SIZE; msg_size <= bench::MAX_MSG_SIZE; msg_size <<= 1) {
            bench::RoundConfig cfg{msg_size, bench::WARMUP_MESSAGES, bench::calculate_messages(msg_size, target_bytes)};

            if (!bench::send_round_config(sock, cfg)) {
                throw std::runtime_error("failed to send round metadata");
            }

            // Warmup is deliberately not timed.
            for (uint64_t i = 0; i < cfg.warmup_messages; ++i) {
                if (!bench::send_all(sock, buffer.data(), cfg.msg_size)) {
                    throw std::runtime_error("failed during warmup send");
                }
            }

            uint32_t ack = 0;
            if (!bench::recv_u32(sock, ack) || ack != 1) {
                throw std::runtime_error("server did not acknowledge warmup");
            }

            const double start = bench::now_seconds();
            for (uint64_t i = 0; i < cfg.measured_messages; ++i) {
                if (!bench::send_all(sock, buffer.data(), cfg.msg_size)) {
                    throw std::runtime_error("failed during measured send");
                }
            }

            if (!bench::recv_u32(sock, ack) || ack != 2) {
                throw std::runtime_error("server did not acknowledge measured data");
            }
            const double end = bench::now_seconds();

            const double seconds = end - start;
            const double bytes = static_cast<double>(cfg.msg_size) * static_cast<double>(cfg.measured_messages);
            const double mib = bytes / (1024.0 * 1024.0);
            const double mbps = (bytes * 8.0) / seconds / 1'000'000.0;

            std::cout << std::left
                      << std::setw(14) << cfg.msg_size
                      << std::setw(14) << cfg.measured_messages
                      << std::setw(16) << std::fixed << std::setprecision(2) << mib
                      << std::setw(14) << std::fixed << std::setprecision(6) << seconds
                      << std::setw(14) << std::fixed << std::setprecision(3) << mbps << "\n";
        }

        bench::RoundConfig stop{0, 0, 0};
        bench::send_round_config(sock, stop);
        bench::close_fd(sock);
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "[-] " << e.what() << "\n";
        return 1;
    }
}
