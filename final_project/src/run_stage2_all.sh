#!/bin/bash

# Compile the refiner
echo "Compiling refiner..."
g++ refiner_main.cpp -o refiner -O3 -std=c++11

if [ $? -ne 0 ]; then
    echo "Compilation failed."
    exit 1
fi

# Loop through cases 1 to 6
for i in {1..6}; do
    # Format number with leading zero (01, 02, ...)
    CASE=$(printf "%02d" $i)
    
    # Define file paths based on your stage2.sh example
    INPUT="../input/case${CASE}-input.txt"
    INITIAL_OUTPUT="../output/ver_09/case${CASE}.out"
    FINAL_OUTPUT="./result_case${CASE}.out"
    IMAGE="case${CASE}.png"
    
    echo "========================================"
    echo "Processing Case $CASE"
    
    if [ -f "$INITIAL_OUTPUT" ]; then
        # Run the refiner
        echo "Running refiner..."
        ./refiner "$INPUT" "$INITIAL_OUTPUT" "$FINAL_OUTPUT"
        
        # Run the visualizer
        echo "Generating visualization..."
        python3 ../visualize.py "$INPUT" "$FINAL_OUTPUT" -o "$IMAGE"
    else
        echo "Warning: Input file $INITIAL_OUTPUT does not exist. Skipping."
    fi
done

echo "========================================"
echo "All cases processed."
