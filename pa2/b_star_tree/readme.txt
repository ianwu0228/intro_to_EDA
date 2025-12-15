=========================================
README - Programming Assignment #2: Fixed-outline Floorplanning
=========================================

Student ID: B11901188
Name: Yi-En Wu
Course: Physical Design for Nanometer ICs, Spring 2025
Instructor: Yao-Wen Chang
Date: April 25th, 2025

-----------------------------------------
1. Compilation
-----------------------------------------
To compile the project, open a terminal and navigate to the root directory that contains the Makefile.

Then simply run:

    make

This will compile the source code located in the `src/` directory and generate an executable named `fp` inside the `bin/` directory.

To clean up all compiled files before recompiling:

    make clean
    make

-----------------------------------------
2. Execution
-----------------------------------------
After compilation, you can run the program using the following command format:

    ./bin/fp <alpha value> <input.block file> <input.net file> <output file> 

Example:

    ./bin/fp 0.5 ./input/ami33.block ./input/ami33.nets ./output/output.rpt

The program reads block and netlist files, performs floorplanning based on simulated annealing and B*-tree representation, and outputs the final floorplan report into the output file.

-----------------------------------------
3. Input Format
-----------------------------------------
- **input.block**: 
  - Outline dimensions
  - Number of blocks
  - Number of terminals
  - List of block names with widths and heights
  - List of terminal names with x, y positions

- **input.nets**:
  - Number of nets
  - For each net: degree and connected terminals

Example (`input.block`):
