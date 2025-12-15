#ifndef FLOORPLANNER_H
#define FLOORPLANNER_H

#include <iostream>
#include <vector>
#include <string>
#include <fstream>
#include <unordered_map>
#include "module.h"
#include "node.h"
#include "tree.h"

using namespace std;

class Floorplanner
{
public:
    // Constructor
    Floorplanner(fstream &inputFile, double alpha)
    {
        _alpha = alpha;
        parseInput(inputFile);
    }

    // Main routine
    void floorplan();

    // Data Members
    double _alpha;          // Weight for Area vs Wirelength
    double _beta = 0.5;     // Weight for Aspect Ratio penalty
    double _gamma = 1000.0; // Weight for Boundary penalty
    double _delta = 1000.0; // Weight for Fixed Overlap penalty

    size_t _chipWidth;  // Fixed Outline Width (from CHIP)
    size_t _chipHeight; // Fixed Outline Height (from CHIP)

    // Separate vectors for Soft and Fixed modules
    vector<Block> _soft_modules;
    vector<Block> _fixed_modules;
    vector<Net> _net_array;

    unordered_map<string, Terminal *> _name2Terminal;

    Tree *_tree;

    // Parsing
    void parseInput(fstream &inputFile);

    // Output results
    size_t getOutputWirelength() const { return outputWirelength; }
    void outputResults(fstream &outputFile, double runtime);

    // Optimization & Cost
    void simulatedAnnealing(); // Added this!

    double computeWirelength();
    double computeArea(); // Added this!
    double computeCost(); // Signature updated to take no args

    // Helper to calculate normalization factors
    void computeNormalizationFactors(double &areaNorm, double &wlNorm,
                                     double &boundaryNorm, double &overlapNorm,
                                     int sampleSize);

    // Penalties
    double computeFixedOverlapPenalty();
    double computeBoundaryPenalty();

private:
    size_t outputWirelength;
    double _normWL = 1.0;
    double _normArea = 1.0;
    double _normBoundary = 1.0;
    double _normOverlap = 1.0;
};

#endif // FLOORPLANNER_H