#include <iostream>
#include <unistd.h>
#include <netinet/in.h>
#include "shared.h"

using namespace std;

int main() {
    // 1. Create a standard TCP socket
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    
    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY; // Accept connections on any network interface
    address.sin_port = htons(PORT);

    // 2. Bind and listen
    if (bind(server_fd, (struct sockaddr*)&address, sizeof(address)) < 0 || listen(server_fd, 3) < 0) {
        cerr << "[-] Bind/Listen failed.\n";
        return 1;
    }

    cout << "[+] Server listening on port " << PORT << "...\n";

    // 3. Accept a client connection
    int addrlen = sizeof(address);
    int client_fd = accept(server_fd, (struct sockaddr*)&address, (socklen_t*)&addrlen);
    cout << "[+] Client connected. Starting rounds...\n";

    // A temporary trash bin buffer where we dump the incoming data
    char storage_buffer[4096]; 

    // 4. Run the exact same loop structure as the client
    for (int i = MIN_EXPONENT; i <= MAX_EXPONENT; ++i) {
        int msg_size = 1 << i;
        size_t X = calculate_X(msg_size);

        // How many bytes total is the client sending in this round?
        size_t warmup_bytes = NUM_WARMUP_MESSAGES * msg_size;
        size_t measurement_bytes = X * msg_size;
        size_t total_expected_bytes = warmup_bytes + measurement_bytes;

        size_t total_bytes_received = 0;

        // Keep reading chunks of data until we hit the exact total expected bytes
        while (total_bytes_received < total_expected_bytes) {
            // Read whatever is currently waiting in the operating system network buffer
            ssize_t bytes_read = read(client_fd, storage_buffer, sizeof(storage_buffer));
            
            if (bytes_read <= 0) {
                cerr << "[-] Client disconnected unexpectedly.\n";
                close(client_fd);
                close(server_fd);
                return 1;
            }
            
            // Add what we just read to our running total
            total_bytes_received += bytes_read;
        }

        // Once we have eaten all the data for this size round, send an ACK back
        // to tell the client it can stop the clock and move on to the next round.
        send(client_fd, "OK", 2, 0);
    }

    cout << "[+] All rounds complete. Cleaning up.\n";
    close(client_fd);
    close(server_fd);
    return 0;
}