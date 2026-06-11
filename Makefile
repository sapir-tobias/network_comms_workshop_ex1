CXX := g++
CXXFLAGS := -std=c++17 -O2 -Wall -Wextra -pedantic

CLIENT := client
SERVER := server

CLIENT_SRC := benchmark_client.cpp
SERVER_SRC := benchmark_server.cpp
SHARED_HDR := benchmark_shared.hpp

.PHONY: all clean run-server run-client

all: $(CLIENT) $(SERVER)

$(CLIENT): $(CLIENT_SRC) $(SHARED_HDR)
	$(CXX) $(CXXFLAGS) $(CLIENT_SRC) -o $(CLIENT)

$(SERVER): $(SERVER_SRC) $(SHARED_HDR)
	$(CXX) $(CXXFLAGS) $(SERVER_SRC) -o $(SERVER)

run-server: $(SERVER)
	./$(SERVER)

run-client: $(CLIENT)
	./$(CLIENT) 127.0.0.1

clean:
	rm -f $(CLIENT) $(SERVER)