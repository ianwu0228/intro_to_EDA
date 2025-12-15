#!/bin/bash

# Output file name
OUTPUT="combined_files.txt"

# Directory to look for files (default to current directory if not provided)
SEARCH_DIR="${1:-src}"

# Remove existing output file
rm -f "$OUTPUT"

# Check if directory exists
if [ ! -d "$SEARCH_DIR" ]; then
    echo "Error: Directory '$SEARCH_DIR' does not exist."
    exit 1
fi

# Loop through all files in the directory
for file in "$SEARCH_DIR"/*; do
    if [ -f "$file" ]; then
        echo "Processing $file..."
        echo "==============================================================================" >> "$OUTPUT"
        echo "File: $file" >> "$OUTPUT"
        echo "==============================================================================" >> "$OUTPUT"
        cat "$file" >> "$OUTPUT"
        echo -e "\n\n" >> "$OUTPUT"
    fi
done

echo "Done! All files from '$SEARCH_DIR' combined into '$OUTPUT'"
