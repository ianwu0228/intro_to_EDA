#!/bin/bash

# Stop the script when any command fails
set -e

# Compile
echo "Compiling..."
make pa3
echo "Compile done."
echo "------------------------"

# Iterate through case1.in to case5.in
for i in 1 2 3 4 5
do
    in_file="public/case${i}.in"
    out_file="public/case${i}.out"

    echo "Running: ./pa3 $in_file $out_file"
    ./pa3 "$in_file" "$out_file"

    echo "Drawing: ./draw $in_file $out_file"
    ./draw "$in_file" "$out_file"

    echo "Case $i finished."
    echo "------------------------"
done

echo "All cases completed!"
