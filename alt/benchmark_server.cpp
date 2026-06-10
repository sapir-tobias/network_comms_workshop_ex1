#include "benchmark_shared.hpp"

int main(int argc, char* argv[]) {
    try {
        if (argc != 1) {
            bench::print_usage_server(argv[0]);
            return 1;
        }

        int server_fd = bench::create_listening_socket();

        sockaddr_in client_addr{};
        socklen_t client_len = sizeof(client_addr);
        int client_fd = accept(server_fd, reinterpret_cast<sockaddr*>(&client_addr), &client_len);
        if (client_fd < 0) bench::throw_errno("accept failed");

        std::vector<char> buffer(64 * 1024);

        while (true) {
            bench::RoundConfig cfg{};
            if (!bench::recv_round_config(client_fd, cfg)) {
                throw std::runtime_error("failed to receive round metadata");
            }

            if (cfg.msg_size == 0) break;
            if (cfg.msg_size > bench::MAX_MSG_SIZE) {
                throw std::runtime_error("client requested message size above MAX_MSG_SIZE");
            }

            const uint64_t warmup_bytes = static_cast<uint64_t>(cfg.msg_size) *
                                          cfg.warmup_messages;
            const uint64_t measured_bytes = static_cast<uint64_t>(cfg.msg_size) *
                                            cfg.measured_messages;

            if (!bench::drain_bytes(client_fd, warmup_bytes, buffer)) {
                throw std::runtime_error("client disconnected during warmup");
            }
            if (!bench::send_u32(client_fd, 1)) {
                throw std::runtime_error("failed to acknowledge warmup");
            }

            if (!bench::drain_bytes(client_fd, measured_bytes, buffer)) {
                throw std::runtime_error("client disconnected during measured round");
            }
            if (!bench::send_u32(client_fd, 2)) {
                throw std::runtime_error("failed to acknowledge measured round");
            }
        }

        bench::close_fd(client_fd);
        bench::close_fd(server_fd);
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "[-] " << e.what() << "\n";
        return 1;
    }
}
