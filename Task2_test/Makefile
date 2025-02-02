
# Compiler and flags
CXX = g++
CXXFLAGS = -std=c++20 -pthread

# Directories for source files and build output
SRCDIR = src
BUILDDIR = build

# Source files
SERVER_SRC = $(SRCDIR)/Server.cpp
CLIENT_SRC = $(SRCDIR)/Client.cpp

# Output executables
SERVER_BIN = $(BUILDDIR)/server
CLIENT_BIN = $(BUILDDIR)/client

# Build targets
all: $(SERVER_BIN) $(CLIENT_BIN)

# Rule to build the server
$(SERVER_BIN): $(SERVER_SRC)
	@mkdir -p $(BUILDDIR)
	$(CXX) $(CXXFLAGS) -o $@ $^

# Rule to build the client
$(CLIENT_BIN): $(CLIENT_SRC)
	@mkdir -p $(BUILDDIR)
	$(CXX) $(CXXFLAGS) -o $@ $^

# Run server and client
run: $(SERVER_BIN) $(CLIENT_BIN)
	@echo "Starting server..."
	$(SERVER_BIN) &
	@echo "Starting client..."
	$(CLIENT_BIN)

# Clean build files
clean:
	rm -rf $(BUILDDIR)

# Phony targets (not associated with files)
.PHONY: all run clean
