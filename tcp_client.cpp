#include <iostream>
#include <cstring>
#include <unistd.h>
#include <arpa/inet.h>
#include "shared.h"

using namespace std;

int main() {
    
    // 1. Create a standard TCP socket
    int client_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (client_fd == -1) {
        cerr << "[-] Socket creation failed.\n";
        return 1;
    }

    // 2. Specify the server address details
    sockaddr_in server_address{};
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(PORT);
    inet_pton(AF_INET, HOST, &server_address.sin_addr);

    // 3. Connect to the server
    if (connect(client_fd, (struct sockaddr*)&server_address, sizeof(server_address)) < 0) {
        cerr << "[-] Connection failed.\n";
        close(client_fd);
        return 1;
    }
    cout << "[+] Connected to server.\n";

    // 4. Run the benchmark loop (1 Byte up to 1 MB)
    for (int i = MIN_EXPONENT; i <= MAX_EXPONENT; ++i) {
        
        int msg_size = 1 << i; 
        size_t X = calculate_X(msg_size); // How many messages to send for this size
        // Allocate a buffer and fill it with dummy characters
        char* message = new char[msg_size];
        memset(message, 'A', msg_size);

        // --- PHASE 1: WARM-UP ---
        // We send a quick burst so the network card wakes up before we start timing
        for (size_t j = 0; j < NUM_WARMUP_MESSAGES; ++j) {
            send(client_fd, message, msg_size, 0);
        }

        // --- PHASE 2: MEASUREMENT ---
        // Start the clock right before we send the benchmark data
        auto start_time = chrono::high_resolution_clock::now();
        
        // Send X messages of the current size
        for (size_t j = 0; j < X; ++j) {
            send(client_fd, message, msg_size, 0);
        }

        // --- PHASE 3: SYNCHRONIZATION ---
        // Wait for an acknowledgment from the server to ensure all data has been received
        char ack_buffer[64] = {0}; // Small buffer for the ACK message
        read(client_fd, ack_buffer, sizeof(ack_buffer));
        
        // Stop the clock
        auto end_time = chrono::high_resolution_clock::now();
        
        delete[] message; // Free memory

        // --- PHASE 4: CALCULATE THROUGHPUT ---
        // Calculate exactly how long it took in seconds
        auto duration_us = chrono::duration_cast<chrono::microseconds>(end_time - start_time).count(); // Duration in microseconds
        double duration_seconds = duration_us / 1000000.0; // Divide by 1,000,000 to convert to seconds

        // Total bits = (Message Size * X) * 8 bits per byte
        double total_bits = static_cast<double>(msg_size) * X * 8.0; 
        
        // Mbps = Total Bits / Seconds / 1,000,000 to convert to Megabits per second
        double throughput_mbps = (total_bits / duration_seconds) / 1000000.0; 

        // Print the results for this message size in the format: "Message Size (Bytes) \t Throughput (Mbps) \t Mbps"
        cout << msg_size << "\t" << throughput_mbps << "\t" << "Mbps\n";
    }

    close(client_fd);
    return 0;
}