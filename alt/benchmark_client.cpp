#include "benchmark_shared.hpp"
#include <iomanip>

int main(int argc, char* argv[]) {
    try {
        if (argc != 2) {
            bench::print_usage_client(argv[0]);
            return 1;
        }

        const std::string host = argv[1];
        int sock = bench::connect_to_server(host);
        std::vector<char> buffer(bench::MAX_MSG_SIZE, 'A');

        for (uint32_t msg_size = bench::MIN_MSG_SIZE;
             msg_size <= bench::MAX_MSG_SIZE;
             msg_size <<= 1) {

            bench::RoundConfig cfg{
                msg_size,
                bench::WARMUP_MESSAGES,
                bench::calculate_messages(msg_size)
            };

            if (!bench::send_round_config(sock, cfg)) {
                throw std::runtime_error("failed to send round metadata");
            }

            // Warm-up phase: not timed. See benchmark_shared.hpp for why 256
            // messages were chosen.
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
            const double bytes = static_cast<double>(cfg.msg_size) *
                                 static_cast<double>(cfg.measured_messages);
            const double mbps = (bytes * 8.0) / seconds / 1000000.0;

            // Required auto-test format: exactly three tab-delimited columns:
            // message size, throughput value, and unit of measurement.
            std::cout << cfg.msg_size << '\t'
                      << std::fixed << std::setprecision(3) << mbps << '\t'
                      << "Mbps" << '\n';
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
