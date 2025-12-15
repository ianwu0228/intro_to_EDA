
CXX = g++
CXXFLAGS = -std=c++11 -O3
TARGET = bin/fp
SRCS = src/main.cpp src/floorplanner.cpp src/tree.cpp
INC = -Isrc/

all: $(TARGET)

$(TARGET): $(SRCS)
	$(CXX) $(CXXFLAGS) $(INC) $(SRCS) -o $(TARGET)

clean:
	rm -rf bin/fp
