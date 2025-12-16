===============================================================================
INTRODUCTION TO EDA - FINAL PROJECT
2023 ICCAD Problem D: Fixed-Outline Floorplanning with Rectilinear Soft Blocks
===============================================================================

## TEAM MEMBER

Name: 吳毅恩、吳頴祈

## PROJECT OVERVIEW

This project implements a two-stage floorplanning algorithm:

1. Stage 1 (Floorplanner): Initial B\*-Tree construction and Simulated
   Annealing.
2. Stage 2 (Refiner): Optimization of the initial layout.

## DIRECTORY STRUCTURE

./
|-- bin/ # Executable directory (created automatically)
|-- input/ # Input test cases (case01-input.txt, etc.)
|-- output/ # Output files (.out) and Visualizations (.png)
|-- src/ # Source codes
|-- visualize.py # Visualization script
|-- run_two_stages.sh # Unified shell script for compilation & execution
|-- readme.txt # This file

## SYSTEM REQUIREMENTS

- OS: Linux
- Compiler: g++ (C++11 standard or higher)
- Python: Python 3.x
- Libraries: matplotlib (for visualization)
  Command: pip3 install matplotlib

## HOW TO COMPILE AND RUN

A unified shell script `run_two_stages.sh` is provided to handle
compilation, execution of both stages, and visualization automatically.

1. Grant execution permission:
   $ chmod +x run_two_stages.sh

2. Run a specific test case (e.g., Case 1):
   $ ./run_two_stages.sh 1

   This command will:

   - Compile the Stage 1 floorplanner (if needed).
   - Compile the Stage 2 refiner (if needed).
   - Run Stage 1 and pipe results to Stage 2.
   - Generate the final output file in `output/case01.out`.
   - Generate the visualization image in `output/case01.png`.

========================================================================
