# ===== Project =====
BIN_DIR   := bin
SRC_DIR   := src
INC_DIR   := include
BUILD_DIR := build

# ===== Gurobi 12.0.3 =====
GRB_DIR := gurobi1203/linux64
GRB_INC := $(GRB_DIR)/include
GRB_LIB := $(GRB_DIR)/lib

# ===== Toolchain =====
CXX      := g++
CXXFLAGS := -O3 -std=c++17 -I$(INC_DIR) -I$(GRB_INC) -MMD -MP
LDFLAGS  := -L$(GRB_LIB) -Wl,-rpath,'$$ORIGIN/../$(GRB_LIB)'
LIBS     := -lgurobi_c++ -lgurobi120 -lpthread -lm

# ===== Binaries =====
TARGET_FP   := $(BIN_DIR)/fp
TARGET_BFP  := $(BIN_DIR)/b_fp
TARGET_FMT  := $(BIN_DIR)/format_exchange

# ===== Source sets =====
COMMON_SRCS := cluster.cpp parser.cpp solver.cpp

FP_SRCS  := main.cpp floorplanner.cpp $(COMMON_SRCS)
BFP_SRCS := b_main.cpp b_floorplanner.cpp b_tree.cpp $(COMMON_SRCS)
FMT_SRCS := format_exchange.cpp

# ===== Object files =====
FP_OBJS  := $(addprefix $(BUILD_DIR)/, $(FP_SRCS:.cpp=.o))
BFP_OBJS := $(addprefix $(BUILD_DIR)/, $(BFP_SRCS:.cpp=.o))
FMT_OBJS := $(addprefix $(BUILD_DIR)/, $(FMT_SRCS:.cpp=.o))

ALL_OBJS := $(FP_OBJS) $(BFP_OBJS) $(FMT_OBJS)
DEPS     := $(ALL_OBJS:.o=.d)

# ===== Phony targets =====
.PHONY: all clean fp bfp format_exchange release

all: $(TARGET_FP) $(TARGET_BFP) $(TARGET_FMT)

fp: $(TARGET_FP)
bfp: $(TARGET_BFP)
format_exchange: $(TARGET_FMT)

# ----- Link binaries -----
$(TARGET_FP): $(FP_OBJS)
	@mkdir -p $(BIN_DIR)
	$(CXX) -o $@ $^ $(LDFLAGS) $(LIBS)

$(TARGET_BFP): $(BFP_OBJS)
	@mkdir -p $(BIN_DIR)
	$(CXX) -o $@ $^ $(LDFLAGS) $(LIBS)

$(TARGET_FMT): $(FMT_OBJS)
	@mkdir -p $(BIN_DIR)
	$(CXX) -o $@ $^ $(LDFLAGS) $(LIBS)

# ----- Compile rules -----
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp
	@mkdir -p $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

# ----- Release build (same 3 binaries, with NDEBUG) -----
release: CXXFLAGS := -O3 -DNDEBUG -std=c++17 -I$(INC_DIR) -I$(GRB_INC) -MMD -MP
release: clean all

# ----- Clean -----
clean:
	rm -rf $(BUILD_DIR) $(BIN_DIR)

-include $(DEPS)
