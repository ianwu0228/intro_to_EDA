CXX := g++
CXXFLAGS := -std=gnu++17 -O2 -Wall -Wextra -Wpedantic

SRC := src/BenchParser.cpp src/main.cpp
INC := -Isrc

BIN_DIR := bin
BIN := $(BIN_DIR)/parse_demo

all: $(BIN)

$(BIN): $(SRC) src/BenchParser.hpp
	@mkdir -p $(BIN_DIR)
	$(CXX) $(CXXFLAGS) $(INC) $(SRC) -o $(BIN)

clean:
	rm -rf $(BIN_DIR)
