// main.cpp

#include <iostream>
#include <fstream>
#include <vector>
#include <ctime>
#include <cstdlib>
#include <chrono>

#include "floorplanner.h"

using namespace std;

int main(int argc, char **argv)
{
    srand(static_cast<unsigned int>(time(0)));
    fstream input_file, output;
    double alpha = 0.5; // Default alpha

    // ICCAD Format: ./fp [input_file] [output_file]
    // Or your Makefile format: ./fp [alpha] [input] [output] (Let's support your Makefile format)

    if (argc == 4)
    {
        cout << "Makefile Format Detected." << endl;
        // format: ./fp <alpha> <input> <output>
        alpha = stod(argv[1]);
        input_file.open(argv[2], ios::in);
        output.open(argv[3], ios::out);

        if (!input_file)
        {
            cerr << "Cannot open input file: " << argv[2] << endl;
            exit(1);
        }
        if (!output)
        {
            cerr << "Cannot open output file: " << argv[3] << endl;
            exit(1);
        }
    }
    // ICCAD Contest format often uses: ./binary input output
    else if (argc == 3)
    {
        cout << "ICCAD Format Detected." << endl;
        input_file.open(argv[1], ios::in);
        output.open(argv[2], ios::out);
        alpha = 0.5; // Default if not provided
    }
    else
    {
        cerr << "Usage: ./Floorplanner <alpha> <input file> <output file>" << endl;
        exit(1);
    }

    // New Constructor: Single input file
    Floorplanner *fp = new Floorplanner(input_file, alpha);
    cout << "Floorplanner initialized with alpha = " << alpha << endl;

    // Start timing
    auto start_time = chrono::high_resolution_clock::now();

    fp->floorplan();

    // End timing
    auto end_time = chrono::high_resolution_clock::now();
    auto duration = chrono::duration_cast<chrono::milliseconds>(end_time - start_time);

    cout << "Time taken: " << duration.count() * 0.001 << " s" << endl;

    // Output results
    fp->outputResults(output, duration.count() * 0.001);

    return 0;
}