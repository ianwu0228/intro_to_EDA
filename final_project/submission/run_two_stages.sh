#!/bin/bash
set -e

# Usage check
if [ -z "$1" ]; then
    echo "Usage: ./run_case_clean.sh <case_number>"
    exit 1
fi

# 1. Configuration
CASE_NUM=$(printf "%02d" "$1")
INPUT_FILE="input/case${CASE_NUM}-input.txt"
STAGE1_OUTPUT="output/case${CASE_NUM}_stage1.out"
FINAL_OUTPUT="output/case${CASE_NUM}.out"
IMAGE_FILE="output/case${CASE_NUM}.png"

# Source definitions
STAGE1_EXE="bin/fp"
STAGE1_SRCS="src/main.cpp src/floorplanner.cpp src/tree.cpp"
STAGE2_EXE="bin/refiner"
STAGE2_SRC="src/refiner_main.cpp"

# Create directories
mkdir -p output bin

echo "========================================"
echo "Processing Case $CASE_NUM"
echo "========================================"

# 2. Compile Stage 1 (Floorplanner)
echo "[Step 0a] Compiling Stage 1..."
g++ -std=c++11 -O3 -Isrc/ $STAGE1_SRCS -o "$STAGE1_EXE"

# 3. Compile Stage 2 (Refiner)
# Only compile if executable missing or source is newer
if [ ! -f "$STAGE2_EXE" ] || [ "$STAGE2_SRC" -nt "$STAGE2_EXE" ]; then
    echo "[Step 0b] Compiling Stage 2..."
    g++ "$STAGE2_SRC" -o "$STAGE2_EXE" -O3 -std=c++11
fi

# 4. Run Stage 1
echo "[Step 1] Running Initial Floorplanner..."
"$STAGE1_EXE" "$INPUT_FILE" "$STAGE1_OUTPUT"

# Check if Stage 1 output exists
if [ ! -f "$STAGE1_OUTPUT" ]; then
    echo "Error: Stage 1 failed to produce output."
    exit 1
fi

# 5. Run Stage 2
echo "[Step 2] Running Refiner..."
"$STAGE2_EXE" "$INPUT_FILE" "$STAGE1_OUTPUT" "$FINAL_OUTPUT"

# 6. Visualization
echo "[Step 3] Generating Visualization..."
if [ -f "visualize.py" ]; then
    python3 visualize.py "$INPUT_FILE" "$FINAL_OUTPUT" -o "$IMAGE_FILE"
    echo "Visualization saved to $IMAGE_FILE"
else
    echo "Warning: visualize.py not found."
fi

echo "========================================"
echo "Success! Case $CASE_NUM finished."