#!/bin/bash

# List of test cases
numbers=(4 5 11 20 100 162 500)

# Process sample case first
echo "Processing test case: sample"
echo "Running floorplanner..."
./bin/fp ./input/sample.in ./spec/sample.spec sample.out
echo "Generating plot..."
python3 plot_floorplan.py ./input/sample.in sample.out
mv floorplan.png sample.png
echo "Generated sample.png"
echo "---"

# Iterate through each number
for num in "${numbers[@]}"; do
    echo "Processing test case: $num"
    
    echo "Running floorplanner..."
    # Run the floorplanner
    ./bin/fp ./input/${num}.in ./spec/${num}.spec ${num}.out
    
    echo "Generating plot..."
    # Run the plot_floorplan.py script
    python3 plot_floorplan.py ./input/${num}.in ${num}.out
    
    # Move the generated floorplan.png to numbered version
    mv floorplan.png ${num}.png
    
    echo "Generated ${num}.png"
    echo "---"
done

echo "All processing completed successfully!"