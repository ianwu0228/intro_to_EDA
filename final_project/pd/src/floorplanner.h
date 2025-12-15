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
    // Constructor: parse input and set up data structures
    Floorplanner(istream& blockFile, istream& netFile, double alpha){
        _alpha = alpha;
        parseBlockFile(blockFile);
        parseNetFile(netFile);
    };

    // The main routine to perform the floorplan
    void floorplan();

// private:
    // -------------------------------------------
    // Data Members
    // -------------------------------------------
    double             _alpha;         // weight factor in cost function
    double             _beta = 0.5;          // weight factor in cost function
    size_t             _outlineWidth;  // outline's width
    size_t             _outlineHeight; // outline's height
    size_t             _numBlocks;     // number of blocks
    size_t             _numTerminals;  // number of terminals
    size_t             _numNets;       // number of nets
    size_t _maxX = 0; // max x coordinate of blocks
    size_t _maxY = 0; // max y coordinate of blocks

    void computeMaxX() 
    {
        size_t maxX = 0;
        for (Block& blk : _block_array) {
            maxX = max(maxX, blk.getX2());
            
        }
        _maxX = maxX;
    }

    void computeMaxY() 
    {
        size_t maxY = 0;
        for (Block& blk : _block_array) {
            maxY = max(maxY, blk.getY2());
            
        }
        _maxY = maxY;
    }

    // Collections of blocks, terminals, and nets
    vector<Block>      _block_array;
    vector<Terminal>   _terminal_array;
    vector<Net>        _net_array;
    vector<Terminal*>  _allTerminal_array; // all terminals (blocks + terminals)
    unordered_map<string, Terminal*> _name2Terminal; // map for fast access to terminals by name

    size_t outputArea;
    size_t outputWirelength;
    size_t outputCost;

    // B*-tree root (points to a Node)
    // Node*              _root;    
    
    Tree*              _tree; // B*-tree structure for the blocks

    // -------------------------------------------
    // Parsing and Initialization
    // -------------------------------------------
    void parseBlockFile(istream& blockFile);
    void parseNetFile(istream& netFile);

    size_t getOutlineWidth() const { return _outlineWidth; }
    size_t getOutlineHeight() const { return _outlineHeight; }
    size_t getOutputArea() const { return outputArea; }
    size_t getOutputWirelength() const { return outputWirelength; }
    size_t getOutputCost() const { return outputCost; }

    // Build an initial B*-tree (simple or random approach)
    // void buildInitialSolution();

    // -------------------------------------------
    // Floorplan Evaluation
    // -------------------------------------------
    // void packFloorplan(Node* root, size_t baseX, size_t baseY);
    // double computeArea();
    // double computeWireLength();
    // double computeCost();
    double computeArea() const;    // compute bounding box area
    double computeWirelength() ;
    double computeCost(double alpha, double beta,
        double areaNorm, double wireNorm, double aspectNorm, double outlineXNorm, double outlineYNorm);
    void computeNormalizationFactors(double& areaNorm, double& wlNorm, double& aspectNorm, double& outlineXNorm, double& outlineYNorm, int sampleSize);
    void logCostMetrics(int iteration, double normA, double normW, double normR, double normX, double normY, double cost);
    // -------------------------------------------
    // Optimization Routines
    // -------------------------------------------
    void simulatedAnnealing();
    // void perturb();
    // void restore();

    // -------------------------------------------
    // Utility & Output
    // -------------------------------------------
    void outputResults();
    void deleteTree(Node* node);
    void printBlocks();
    void printTerminals();
    void printNets();
    void printAll();
};

#endif // FLOORPLANNER_H
