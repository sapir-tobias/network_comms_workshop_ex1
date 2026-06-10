## Build

```bash
mkdir -p build
cd build
cmake ..
cmake --build .
```

Or directly:

```bash
g++ -std=c++17 -O2 -Wall -Wextra -pedantic benchmark_server.cpp -o benchmark_server
g++ -std=c++17 -O2 -Wall -Wextra -pedantic benchmark_client.cpp -o benchmark_client
```

## Run

Terminal 1:

```bash
./benchmark_server 8080
```

Terminal 2:

```bash
./benchmark_client 127.0.0.1 8080 256
```

Arguments:

```text
benchmark_server [port]
benchmark_client [server-host] [port] [target-MiB-per-size]
```

Use `256` MiB per message size for stable benchmark results. For a quick smoke test only, you can use `0` or a very small value, but those numbers are not meaningful benchmark results.

## Design choices

- Uses explicit round metadata: message size, warmup messages, measured messages.
- Uses `send_all` and `recv_all` to handle partial TCP sends/receives correctly.
- Uses `steady_clock` for timing.
- Uses per-message-size target volume, so large and small message sizes are tested more fairly.
- Keeps warmup traffic outside the measured timing.
- Server ACKs after warmup and after measured traffic, so the client stops timing only after the server actually received the data.
- Supports hostnames and IPs, IPv4 and IPv6.
- Uses `SO_REUSEADDR` so restarting the server is less annoying.
