#!/bin/bash

# Exit immediately if a command exits with a non-zero status
set -e

# Check if a case number argument is provided
if [ -z "$1" ]; then
    echo "Usage: ./run_case.sh <case_number>"
    echo "Example: ./run_case.sh 3"
    exit 1
fi

# Format the case number (e.g., 3 -> 03)
CASE_NUM=$(printf "%02d" "$1")

# ==========================================
# Configuration / Paths
# ==========================================
INPUT_FILE="input/case${CASE_NUM}-input.txt"
STAGE1_OUTPUT="output/case${CASE_NUM}_stage1.out"
FINAL_OUTPUT="output/case${CASE_NUM}.out"
IMAGE_FILE="output/case${CASE_NUM}.png"

# Stage 1 Source Files
STAGE1_TARGET="bin/fp"
STAGE1_SRCS="src/main.cpp src/floorplanner.cpp src/tree.cpp"

# Stage 2 Source File
REFINER_TARGET="bin/refiner"
REFINER_SRC="src/refiner_main.cpp" 

# Ensure directories exist
mkdir -p output
mkdir -p bin

echo "========================================"
echo "Processing Case $CASE_NUM"
echo "Input: $INPUT_FILE"
echo "========================================"

# ==========================================
# Step 0a: Compile Stage 1 (Floorplanner)
# ==========================================
echo "[Step 0a] Compiling Stage 1 (Floorplanner)..."
# Using flags and sources from your Makefile snippet
g++ -std=c++11 -O3 -Isrc/ $STAGE1_SRCS -o $STAGE1_TARGET

if [ $? -eq 0 ]; then
    echo "Stage 1 compilation successful."
else
    echo "Error: Stage 1 compilation failed."
    exit 1
fi

# ==========================================
# Step 0b: Compile Stage 2 (Refiner)
# ==========================================
# Only compile if executable is missing or source is newer
if [ ! -f "$REFINER_TARGET" ] || [ "$REFINER_SRC" -nt "$REFINER_TARGET" ]; then
    echo "[Step 0b] Compiling Stage 2 (Refiner)..."
    if [ -f "$REFINER_SRC" ]; then
        g++ "$REFINER_SRC" -o "$REFINER_TARGET" -O3 -std=c++11
    else
        echo "Error: $REFINER_SRC not found."
        exit 1
    fi
else
    echo "[Step 0b] Refiner already up to date."
fi

# ==========================================
# Step 1: Run Floorplanner (Stage 1)
# ==========================================
echo "[Step 1] Running Initial Floorplanner..."
$STAGE1_TARGET "$INPUT_FILE" "$STAGE1_OUTPUT"

if [ $? -eq 0 ]; then
    echo "Stage 1 completed. Output: $STAGE1_OUTPUT"
else
    echo "Error: Stage 1 (Floorplanner) execution failed."
    exit 1
fi

# ==========================================
# Step 2: Run Refiner (Stage 2)
# ==========================================
echo "[Step 2] Running Refiner..."
# Usage: ./refiner <original_input> <stage1_output> <final_output>
$REFINER_TARGET "$INPUT_FILE" "$STAGE1_OUTPUT" "$FINAL_OUTPUT"

if [ $? -eq 0 ]; then
    echo "Stage 2 completed. Final Output: $FINAL_OUTPUT"
else
    echo "Error: Stage 2 (Refiner) execution failed."
    exit 1
fi

# ==========================================
# Step 3: Visualization
# ==========================================
echo "[Step 3] Generating Visualization..."
if [ -f "visualize.py" ]; then
    python3 visualize.py "$INPUT_FILE" "$FINAL_OUTPUT" -o "$IMAGE_FILE"
    echo "Visualization saved to $IMAGE_FILE"
else
    echo "Warning: visualize.py not found. Skipping visualization."
fi

echo "========================================"
echo "Success! Case $CASE_NUM processed completely."