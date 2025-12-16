#!/bin/bash

# Create output directory if it doesn't exist
mkdir -p output

# Loop through cases 1 to 6
for i in {3..3}
do
    # Format the number with leading zero (e.g., 01, 02)
    CASE_NUM=$(printf "%02d" $i)
    
    INPUT_FILE="input/case${CASE_NUM}-input.txt"
    OUTPUT_FILE="output/case${CASE_NUM}.out"
    IMAGE_FILE="output/case${CASE_NUM}.png"

    echo "----------------------------------------------------------------"
    echo "Running Case $CASE_NUM"
    echo "Input: $INPUT_FILE"
    echo "Output: $OUTPUT_FILE"
    
    # Run the floorplanner
    # Usage: ./bin/fp <input_file> <output_file>
    ./bin/fp "$INPUT_FILE" "$OUTPUT_FILE"
    
    # Check if floorplanner succeeded
    if [ $? -eq 0 ]; then
        echo "Floorplanning successful. Generating visualization..."
        
        # Run the visualizer
        # Usage: python3 visualize.py <input_file> <output_file> -o <image_file>
        python3 visualize.py "$INPUT_FILE" "$OUTPUT_FILE" -o "$IMAGE_FILE"
        
        if [ $? -eq 0 ]; then
            echo "Visualization saved to $IMAGE_FILE"
        else
            echo "Error: Visualization failed for Case $CASE_NUM"
        fi
    else
        echo "Error: Floorplanner failed for Case $CASE_NUM"
    fi
    echo "----------------------------------------------------------------"
    echo ""
done

echo "All cases processed."
