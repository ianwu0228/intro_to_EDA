// #ifndef REFINE_H
// #define REFINE_H

// #include <iostream>
// #include <vector>
// #include <string>
// #include <fstream>
// #include <unordered_map>
// #include <algorithm>
// #include <cmath>
// #include <climits>
// #include <iomanip>

// using namespace std;

// // Internal structures strictly for the Refiner
// // (Independent of Stage 1 definitions)
// struct RefinePoint
// {
//     int x, y;
// };

// struct RefineBlock
// {
//     int id;
//     string name;
//     size_t minArea;
//     bool isFixed;

//     // Bounding Box (for constraint checking)
//     int x1, y1, x2, y2;

//     // Center of Mass (for force calculation)
//     double cx, cy;

//     // Movement tendency
//     double forceX;
//     double forceY;

//     // Area tracking
//     int currentArea;
// };

// struct RefineNet
// {
//     vector<int> moduleIds;
// };

// class Refiner
// {
// public:
//     // Constructor takes filenames to remain decoupled from Stage 1 memory
//     Refiner(string inputFileName, string stage1OutputFileName, string outputFileName);
//     ~Refiner();

//     // The main driver function
//     void refine();

// private:
//     string _inputFileName;
//     string _stage1OutputFileName;
//     string _outputFileName;

//     int _chipW, _chipH;

//     // Self-contained data storage
//     vector<RefineBlock> _modules;
//     vector<RefineNet> _nets;
//     unordered_map<string, int> _nameToId;

//     // The Grid: stores module ID at [x][y], -1 if empty
//     // Using vector<vector<int>> for simplicity and safety
//     vector<vector<int>> _grid;

//     // File Parsing
//     void parseInput();  // Reads problem constraints (.txt)
//     void parseStage1(); // Reads initial placement (.out)

//     // Grid Management
//     void initGrid();
//     bool isInside(int x, int y) const;

//     // Physics/Expansion Logic
//     void calculateForces();
//     void expandModules();
//     bool tryExpand(int modIdx, int dirX, int dirY);

//     // Constraint Checking
//     // Checks Area, Aspect Ratio (0.5-2.0), and Rectangle Ratio (>= 0.8)
//     bool checkConstraints(const RefineBlock &mod, int addedArea, int newX1, int newY1, int newX2, int newY2);

//     // Output Generation
//     void writeOutput();
//     double calculateFinalHPWL();
//     vector<RefinePoint> getClockwiseCorners(int modIdx);
// };

// #endif // REFINE_H