CXX := g++
CXXFLAGS := -std=gnu++17 -O2 -Wall -Wextra -Wpedantic
INC := -Isrc
BIN := bin

all: $(BIN)/ec

$(BIN)/parse_demo): src/BenchParser.cpp src/BenchParser.hpp src/main.cpp
	@mkdir -p $(BIN)
	$(CXX) $(CXXFLAGS) $(INC) src/BenchParser.cpp src/main.cpp -o $(BIN)/parse_demo

$(BIN)/ec: src/BenchParser.cpp src/BenchParser.hpp src/CNF.hpp src/Tseitin.hpp src/main.cpp
	@mkdir -p $(BIN)
	$(CXX) $(CXXFLAGS) $(INC) src/BenchParser.cpp src/main.cpp -o $(BIN)/ec

clean:
	rm -rf $(BIN)
