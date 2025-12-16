#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include <cmath>
#include <algorithm>
#include <iomanip>
#include "floorplanner.h"

using namespace std;

// 1. Parsing Logic
void Floorplanner::parseInput(fstream &inputFile)
{
    string keyword;

    if (!(inputFile >> keyword) || keyword != "CHIP")
    {
        cerr << "Error: Expected 'CHIP'" << endl;
        exit(1);
    }
    inputFile >> _chipWidth >> _chipHeight;

    int numSoft;
    if (!(inputFile >> keyword) || keyword != "SOFTMODULE")
    {
        cerr << "Error: Expected 'SOFTMODULE'" << endl;
        exit(1);
    }
    inputFile >> numSoft;
    _soft_modules.reserve(numSoft);

    for (int i = 0; i < numSoft; ++i)
    {
        string name;
        size_t area;
        inputFile >> name >> area;
        Block b(name, area);
        b.setID(i);
        _soft_modules.push_back(b);
    }

    // ==========================================
    // NEW: INJECT GHOST BLOCKS
    // ==========================================
    // Configuration: 15% of soft modules, sized at 50% average area
    if (numSoft > 0)
    {
        int numGhosts = static_cast<int>(numSoft * 0.15);

        // Calculate average area
        double totalArea = 0;
        for (const auto &b : _soft_modules)
            totalArea += b.getMinArea();
        size_t avgArea = totalArea / numSoft;
        size_t ghostArea = avgArea / 2;
        if (ghostArea < 1)
            ghostArea = 1; // Safety

        for (int i = 0; i < numGhosts; ++i)
        {
            string name = "GHOST_" + to_string(i);
            // Create ghost block: area, isGhost=true
            Block b(name, ghostArea, true);
            // Assign ID (append to end of list)
            b.setID(numSoft + i);
            _soft_modules.push_back(b);
        }
    }
    // ==========================================

    int numFixed;
    if (!(inputFile >> keyword) || keyword != "FIXEDMODULE")
    {
        cerr << "Error: Expected 'FIXEDMODULE'" << endl;
        exit(1);
    }
    inputFile >> numFixed;
    _fixed_modules.reserve(numFixed);

    for (int i = 0; i < numFixed; ++i)
    {
        string name;
        size_t x, y, w, h;
        inputFile >> name >> x >> y >> w >> h;
        Block b(name, w, h, x, y);
        b.setID(numSoft + i);
        _fixed_modules.push_back(b);
    }

    // Build Map (Wait until vectors are filled so pointers don't invalidate)
    for (auto &b : _soft_modules)
        _name2Terminal[b.getName()] = &b;
    for (auto &b : _fixed_modules)
        _name2Terminal[b.getName()] = &b;

    int numConnections;
    if (!(inputFile >> keyword) || keyword != "CONNECTION")
    {
        cerr << "Error: Expected 'CONNECTION'" << endl;
        exit(1);
    }
    inputFile >> numConnections;

    _net_array.clear();
    for (int i = 0; i < numConnections; ++i)
    {
        string name1, name2;
        int qty;
        inputFile >> name1 >> name2 >> qty;

        Terminal *t1 = _name2Terminal[name1];
        Terminal *t2 = _name2Terminal[name2];

        if (!t1 || !t2)
        {
            cerr << "Error: Unknown module " << name1 << " or " << name2 << endl;
            continue;
        }

        // Create 'qty' nets (Standard HPWL approach)
        for (int k = 0; k < qty; ++k)
        {
            Net net;
            net.setDegree(2);
            net.addTerm(t1);
            net.addTerm(t2);
            _net_array.push_back(net);
        }
    }

    // Initialize Tree with ONLY Soft Blocks
    _tree = new Tree(_soft_modules);

    // Pass the fixed modules to the tree for contour initialization
    _tree->setFixedModules(&_fixed_modules);
}

void Floorplanner::floorplan()
{
    _tree->buildInitial();
    simulatedAnnealing();
}

// 2. Cost Calculation
double Floorplanner::computeArea()
{
    // Current bounding box area of soft modules
    size_t minX = _chipWidth, maxX = 0;
    size_t minY = _chipHeight, maxY = 0;

    if (_soft_modules.empty())
        return 0;

    for (const auto &blk : _soft_modules)
    {
        minX = min(minX, blk.getX1());
        maxX = max(maxX, blk.getX2());
        minY = min(minY, blk.getY1());
        maxY = max(maxY, blk.getY2());
    }
    return (double)(maxX - minX) * (maxY - minY);
}

// double Floorplanner::computeWirelength()
// {
//     double total = 0.0;
//     for (Net &net : _net_array)
//     {
//         total += net.calcHPWL();
//     }
//     return total;
// }

double Floorplanner::computeWirelength()
{
    double total = 0.0;

    // Temporarily apply offsets to blocks for calculation
    for (auto &soft : _soft_modules)
    {
        soft.setPos(soft.getX1() + _offsetX, soft.getY1() + _offsetY,
                    soft.getX2() + _offsetX, soft.getY2() + _offsetY);
    }

    // Calculate HPWL with absolute coordinates
    for (Net &net : _net_array)
    {
        total += net.calcHPWL();
    }

    // Revert offsets (restore relative coordinates for B*-tree packing)
    // Note: B*-tree pack() will overwrite these anyway, but it's good practice.
    for (auto &soft : _soft_modules)
    {
        soft.setPos(soft.getX1() - _offsetX, soft.getY1() - _offsetY,
                    soft.getX2() - _offsetX, soft.getY2() - _offsetY);
    }

    return total;
}

double Floorplanner::computeFixedOverlapPenalty()
{
    double totalOverlap = 0;
    for (const auto &soft : _soft_modules)
    {
        if (soft.isGhost())
            continue;

        // APPLY OFFSET HERE
        size_t sx1 = soft.getX1() + _offsetX;
        size_t sx2 = soft.getX2() + _offsetX;
        size_t sy1 = soft.getY1() + _offsetY;
        size_t sy2 = soft.getY2() + _offsetY;

        for (const auto &fixed : _fixed_modules)
        {
            // Fixed blocks are absolute, NO offset
            size_t fx1 = fixed.getX1();
            size_t fx2 = fixed.getX2();
            size_t fy1 = fixed.getY1();
            size_t fy2 = fixed.getY2();

            size_t ix1 = max(sx1, fx1);
            size_t ix2 = min(sx2, fx2);
            size_t iy1 = max(sy1, fy1);
            size_t iy2 = min(sy2, fy2);

            if (ix1 < ix2 && iy1 < iy2)
            {
                totalOverlap += (double)(ix2 - ix1) * (iy2 - iy1);
            }
        }
    }
    return totalOverlap;
}

double Floorplanner::computeBoundaryPenalty()
{
    double totalViolation = 0;
    for (const auto &soft : _soft_modules)
    {
        // APPLY OFFSET HERE
        size_t sx2 = soft.getX2() + _offsetX;
        size_t sy2 = soft.getY2() + _offsetY;

        // Check violations
        if (sx2 > _chipWidth)
            totalViolation += (sx2 - _chipWidth) * soft.getHeight();
        if (sy2 > _chipHeight)
            totalViolation += (sy2 - _chipHeight) * soft.getWidth();
        // Also check if offset pushed it negative (though we clamped it to 0 above)
    }
    return totalViolation;
}

double Floorplanner::computeCost()
{
    double W = computeWirelength();
    double Area = computeArea(); // Optional, but usually good to keep area tight
    double Boundary = computeBoundaryPenalty();
    double Overlap = computeFixedOverlapPenalty();

    // Avoid division by zero
    double nW = (_normWL > 0) ? _normWL : 1.0;
    double nA = (_normArea > 0) ? _normArea : 1.0;
    double nB = (_normBoundary > 0) ? _normBoundary : 1.0;
    double nO = (_normOverlap > 0) ? _normOverlap : 1.0;

    // Cost = alpha*Area + (1-alpha)*WL + Penalties
    // Note: The contest only cares about WL, but during annealing we need Area to guide packing.
    // _gamma and _delta should be very large.

    return _alpha * (Area / nA) + (1.0 - _alpha) * (W / nW) + _gamma * (Boundary / nB) + _delta * (Overlap / nO);
}

void Floorplanner::computeNormalizationFactors(double &areaNorm, double &wlNorm,
                                               double &boundaryNorm, double &overlapNorm, int sampleSize)
{
    double totalArea = 0, totalWL = 0, totalBound = 0, totalOverlap = 0;

    for (int i = 0; i < sampleSize; ++i)
    {
        _tree->rotateRandom();
        _tree->deleteAndInsert();
        _tree->resizeRandom(); // Don't forget resize!
        _tree->pack();

        totalArea += computeArea();
        totalWL += computeWirelength();
        totalBound += computeBoundaryPenalty();
        totalOverlap += computeFixedOverlapPenalty();
    }

    areaNorm = totalArea / sampleSize;
    wlNorm = totalWL / sampleSize;
    // Add small epsilon to avoid zero
    boundaryNorm = (totalBound / sampleSize) + 1.0;
    overlapNorm = (totalOverlap / sampleSize) + 1.0;
}

// 3. Simulated Annealing
void Floorplanner::simulatedAnnealing()
{
    double T = 10000.0;
    const double T_min = 0.1;
    const double cooling_rate = 0.98;
    const int iterations = 500; // Increase for better quality

    // Normalization
    computeNormalizationFactors(_normArea, _normWL, _normBoundary, _normOverlap, 50);

    _tree->pack();
    double prevCost = computeCost();
    double bestCost = prevCost;

    // We must save the "best state". Since Tree holds pointers to _soft_modules,
    // we need to save the Tree structure AND the Block dimensions (because resize changes them).
    Tree bestTree = *_tree;
    vector<Block> bestBlocks = _soft_modules; // Save block states (dims)

    while (T > T_min)
    {
        for (int i = 0; i < iterations; ++i)
        {
            Tree backupTree = *_tree;
            vector<Block> backupBlocks = _soft_modules; // Expensive copy, optimize later if needed

            double oldCost = prevCost;

            // Perturb
            double r = (double)rand() / RAND_MAX;

            // Backup offsets
            int backupX = _offsetX;
            int backupY = _offsetY;

            if (r < 0.1)
                _tree->resizeRandom();
            else if (r < 0.3)
                _tree->rotateRandom();
            else if (r < 0.5)
                _tree->swapRandomNodes();
            else if (r < 0.7)
                _tree->deleteAndInsert();
            else
                moveCluster();

            _tree->pack();
            double newCost = computeCost();
            double delta = newCost - oldCost;

            bool accept = (delta < 0) || ((double)rand() / RAND_MAX < exp(-delta / T));

            if (accept)
            {
                prevCost = newCost;
                if (newCost < bestCost)
                {
                    bestCost = newCost;
                    bestTree = *_tree;
                    bestBlocks = _soft_modules;
                }
            }
            else
            {
                _offsetX = backupX; // Restore offset if rejected
                _offsetY = backupY;
                *_tree = backupTree;
                _soft_modules = backupBlocks; // Restore dimensions
            }
        }
        T *= cooling_rate;
    }

    // Restore Best
    *_tree = bestTree;
    _soft_modules = bestBlocks;
    _tree->pack();
    outputWirelength = (size_t)computeWirelength();
}

// // 4. Output Logic (ICCAD Format)
// void Floorplanner::outputResults(fstream &outputFile, double runtime)
// {
//     // ICCAD Format:
//     // HPWL <value>
//     // SOFTMODULE <num>
//     // <name> <num_corners>
//     // <x> <y> ... (corners)

//     // We strictly output rectangles (4 corners) for soft modules

//     outputFile << "HPWL " << fixed << setprecision(1) << (double)outputWirelength << endl;

//     outputFile << "SOFTMODULE " << _soft_modules.size() << endl;
//     for (const auto &b : _soft_modules)
//     {
//         outputFile << b.getName() << " 4" << endl;
//         // Output 4 corners clockwise starting from bottom-left?
//         // Contest usually accepts: (x1,y1) (x1,y2) (x2,y2) (x2,y1)
//         // Check strict contest rules. Assuming standard rectangle output:
//         outputFile << b.getX1() << " " << b.getY1() << endl;
//         outputFile << b.getX1() << " " << b.getY2() << endl;
//         outputFile << b.getX2() << " " << b.getY2() << endl;
//         outputFile << b.getX2() << " " << b.getY1() << endl;
//     }
// }

// void Floorplanner::outputResults(fstream &outputFile, double runtime)
// {
//     // 1. Ensure the tree is packed one last time to get clean relative coordinates
//     _tree->pack();

//     // 2. Output Header
//     // "HPWL <value>" (Precision to 1 decimal place)
//     // Note: We re-calculate HPWL here to ensure it includes the final offsets
//     double finalHPWL = computeWirelength();
//     outputFile << "HPWL " << fixed << setprecision(1) << finalHPWL << endl;

//     // 3. Output Soft Modules
//     // "SOFTMODULE <number>"
//     outputFile << "SOFTMODULE " << _soft_modules.size() << endl;

//     for (const auto &b : _soft_modules)
//     {
//         // Format: <Name> <NumCorners>
//         outputFile << b.getName() << " 4" << endl;

//         // Apply the global Cluster Offset to get absolute coordinates
//         size_t x1 = b.getX1() + _offsetX;
//         size_t y1 = b.getY1() + _offsetY;
//         size_t x2 = b.getX2() + _offsetX;
//         size_t y2 = b.getY2() + _offsetY;

//         //[cite_start] // Output 4 corners in CLOCKWISE order (starting Bottom-Left) [cite: 250]
//         // 1. Bottom-Left
//         outputFile << x1 << " " << y1 << endl;
//         // 2. Top-Left
//         outputFile << x1 << " " << y2 << endl;
//         // 3. Top-Right
//         outputFile << x2 << " " << y2 << endl;
//         // 4. Bottom-Right
//         outputFile << x2 << " " << y1 << endl;
//     }
// }

void Floorplanner::outputResults(fstream &outputFile, double runtime)
{
    // 1. Ensure the tree is packed one last time
    _tree->pack();

    // 2. Output Header
    double finalHPWL = computeWirelength();
    outputFile << "HPWL " << fixed << setprecision(1) << finalHPWL << endl;

    // NEW: Calculate REAL module count (exclude ghosts)
    size_t realCount = 0;
    for (const auto &b : _soft_modules)
    {
        if (!b.isGhost())
            realCount++;
    }

    outputFile << "SOFTMODULE " << realCount << endl;

    for (const auto &b : _soft_modules)
    {
        // NEW: Skip ghost blocks
        if (b.isGhost())
            continue;

        // Format: <Name> <NumCorners>
        outputFile << b.getName() << " 4" << endl;

        // Apply global Cluster Offset
        size_t x1 = b.getX1() + _offsetX;
        size_t y1 = b.getY1() + _offsetY;
        size_t x2 = b.getX2() + _offsetX;
        size_t y2 = b.getY2() + _offsetY;

        // Output 4 corners in CLOCKWISE order (starting Bottom-Left)
        outputFile << x1 << " " << y1 << endl;
        outputFile << x1 << " " << y2 << endl;
        outputFile << x2 << " " << y2 << endl;
        outputFile << x2 << " " << y1 << endl;
    }
}

// Add this new function
void Floorplanner::moveCluster()
{
    // Move the cluster randomly within the chip range
    // We can shift by large amounts or small deltas.
    // For simplicity, let's pick a random valid coordinate occasionally,
    // or drift it slightly.

    // Random drift: -100 to +100 units
    int driftX = (rand() % 200) - 100;
    int driftY = (rand() % 200) - 100;

    _offsetX += driftX;
    _offsetY += driftY;

    // Keep it somewhat sane (optional, but helps convergence)
    if (_offsetX < 0)
        _offsetX = 0;
    if (_offsetY < 0)
        _offsetY = 0;
    if (_offsetX > (int)_chipWidth)
        _offsetX = _chipWidth;
    if (_offsetY > (int)_chipHeight)
        _offsetY = _chipHeight;
}