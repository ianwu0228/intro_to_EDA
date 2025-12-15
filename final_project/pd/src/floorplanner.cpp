#include <iostream>
#include <string>
#include <cstdlib>
#include "floorplanner.h"

using namespace std;

void Floorplanner::parseBlockFile(istream& blockFile)
{

    string keyword;
    blockFile >> keyword;  // should be "Outline:"
    cout << "start parsing block file" << endl;
    cout << keyword << endl;
    if (keyword != "Outline:")
    {
        cerr << "Error: expected 'Outline:' but got '" << keyword << "'\n";
        exit(1);
    }

    blockFile >> _outlineWidth >> _outlineHeight;

    blockFile >> keyword; // should be "NumBlocks:"
    if (keyword != "NumBlocks:")
    {
        cerr << "Error: expected 'NumBlocks:' but got '" << keyword << "'\n";
        exit(1);
    }

    blockFile >> _numBlocks;

    blockFile >> keyword; // should be "NumTerminals:"
    if (keyword != "NumTerminals:")
    {
        cerr << "Error: expected 'NumTerminals:' but got '" << keyword << "'\n";
        exit(1);
    }

    ////////////////////// blocks //////////////// 
    blockFile >> _numTerminals;

    _block_array.reserve(_numBlocks); // Optional, for efficiency
    for (size_t i = 0; i < _numBlocks; ++i)
    {
        string blockName;
        size_t w, h;
        blockFile >> blockName >> w >> h;

        Block newBlock(blockName, w, h);
        newBlock.setID(i); // Set ID for the block
        _block_array.push_back(newBlock);
        _allTerminal_array.push_back(&_block_array[i]); // Add block to all terminals array
    }

    /////////////////// terminals ////////////////
    _terminal_array.reserve(_numTerminals);
    for (size_t i = 0; i < _numTerminals; ++i)
    {
        string termName, terminalKeyword;
        size_t x, y;
        blockFile >> termName >> terminalKeyword >> x >> y;

        if (terminalKeyword != "terminal")
        {
            cerr << "Error: expected 'terminal' but got '" << terminalKeyword << "'\n";
            exit(1);
        }

        Terminal newTerminal(termName, x, y);
        newTerminal.setID(i); // Set ID for the terminal
        _terminal_array.push_back(newTerminal);
        _allTerminal_array.push_back(&_terminal_array[i]); // Add terminal to all terminals array
    }

    for(Terminal* t: _allTerminal_array)
    {
        _name2Terminal[t->getName()] = t; // Map terminal names to Terminal objects
    }
   
    _tree = new Tree(_block_array);

}



void Floorplanner::parseNetFile(istream& netFile)
{
    string kw;
    if (!(netFile >> kw) || kw != "NumNets:") {
        cerr << "Error: expected \"NumNets:\" but got \"" << kw << "\"\n";
        exit(1);
    }
    netFile >> _numNets;

    // --- build local lookup ---
    unordered_map<string, Terminal*> name2ptr;
    name2ptr.reserve(_allTerminal_array.size());
    for (Terminal* t : _allTerminal_array)
        name2ptr[t->getName()] = t;

    _net_array.clear();
    _net_array.reserve(_numNets);

    for (size_t n = 0; n < _numNets; ++n) {
        if (!(netFile >> kw) || kw != "NetDegree:") {
            cerr << "Error: expected \"NetDegree:\" (net " << n << ")\n";
            exit(1);
        }
        size_t deg;  netFile >> deg;

        Net net;
        net.setDegree(deg);

        for (size_t k = 0; k < deg; ++k) {
            string name; netFile >> name;
            auto it = name2ptr.find(name);
            if (it == name2ptr.end()) {
                cerr << "Error: name \"" << name << "\" not found (net " << n << ")\n";
                exit(1);
            }
            net.addTerm(it->second);
        }
        _net_array.push_back(std::move(net));
    }
}



void Floorplanner::printBlocks()
{   
    cout << "===================================" << endl;
    cout << "Blocks = " << _numBlocks << endl;
    cout << "Name Width Height" << endl;
    cout << "------------------" << endl;

    for(int i = 0; i < _block_array.size(); i++)
    {
        cout << _block_array[i].getName() << " " << _block_array[i].getWidth() << " " << _block_array[i].getHeight() << endl;
    }
    cout << "===================================" << endl;
}

void Floorplanner::printTerminals()
{
    cout << "===================================" << endl;
    cout << "Terminals = " << _numTerminals << endl;
    cout << "Name X Y" << endl;
    cout << "------------------" << endl;

    for(int i = 0; i < _terminal_array.size(); i++)
    {
        cout << _terminal_array[i].getName() << " " << _terminal_array[i].getX1() << " " << _terminal_array[i].getY1() << endl;
    }
    cout << "===================================" << endl;
}

void Floorplanner::printNets()
{
    cout << "===================================" << endl;
    cout << "Nets = " << _numNets << endl;
    cout << "Name Terminals" << endl;
    cout << "------------------" << endl;
    cout << "netarray size = " << _net_array.size() << endl;
    for(int i = 0; i < _net_array.size(); i++)
    {
        cout << "net " << i << " " << _net_array[i].getDegree() << " ";
        for(int j = 0; j < _net_array[i].getDegree(); j++)
        {
            cout << _net_array[i].getTermList()[j]->getName() << " ";
        }
        cout << endl;
    }
    cout << "===================================" << endl;
}

void Floorplanner::printAll()
{
    printBlocks();
    printTerminals();
    printNets();
}


void Floorplanner::floorplan()
{

    _tree->buildInitial(); 
    simulatedAnnealing();

    
}

double Floorplanner::computeArea() const {
    size_t maxX = 0, maxY = 0;
    for (Block blk : _block_array) {
        maxX = max(maxX, blk.getX2());
        maxY = max(maxY, blk.getY2());
    }
    return static_cast<double>(maxX) * maxY;
}


double Floorplanner::computeWirelength()  {
    double total = 0.0;
    for (Net& net : _net_array) {
        total += net.calcHPWL();
    }
    return total;
}


// double Floorplanner::computeCost(double alpha, double beta,
//                                  double areaNorm, double wireNorm, double aspectNorm) {
//     double A = computeArea();         
//     double W = computeWirelength();  

//     size_t maxX = 0, maxY = 0;
//     for (Block& blk : _block_array) {
//         maxX = std::max(maxX, blk.getX2());
//         maxY = std::max(maxY, blk.getY2());
//     }

//     if (maxX == 0 || maxY == 0 || areaNorm == 0 || wireNorm == 0 || aspectNorm == 0) return 1e9;

//     double R = static_cast<double>(maxY) / maxX;
//     double R_star = static_cast<double>(_outlineHeight) / _outlineWidth;
//     double aspectPenalty = std::pow(R - R_star, 2.0);

//     double normA = A / areaNorm;
//     double normW = W / wireNorm;
//     double normR = aspectPenalty / aspectNorm;

//     double aspectWeight = beta;

//     double cost = alpha * normA + (1-_alpha) * normW + beta * normR;
//     return cost;
// }

// double Floorplanner::computeCost(double alpha, double beta,
//     double areaNorm, double wireNorm, double aspectNorm) {
// static int iteration = 0;

// double A = computeArea();         
// double W = computeWirelength();  

// size_t maxX = 0, maxY = 0;
// for (Block& blk : _block_array) {
// maxX = std::max(maxX, blk.getX2());
// maxY = std::max(maxY, blk.getY2());
// }

// if (maxX == 0 || maxY == 0 || areaNorm == 0 || wireNorm == 0 || aspectNorm == 0) return 1e9;

// double R = static_cast<double>(maxY) / maxX;
// double R_star = static_cast<double>(_outlineHeight) / _outlineWidth;
// double aspectPenalty = std::pow(R - R_star, 2.0);

// double normA = A / areaNorm;
// double normW = W / wireNorm;
// double normR = aspectPenalty / aspectNorm;

// double cost = alpha * normA + (1-alpha) * normW + beta * normR;

// // Log the metrics
// logCostMetrics(iteration++, normA, normW, normR, cost);

// return cost;
// }
double Floorplanner::computeCost(double alpha, double beta,
    double areaNorm, double wireNorm, double aspectNorm,
    double outlineXNorm, double outlineYNorm) {
    static int iteration = 0;

    double A = computeArea();         
    double W = computeWirelength();  

    size_t maxX = 0, maxY = 0;
    for (Block& blk : _block_array) {
        maxX = std::max(maxX, blk.getX2());
        maxY = std::max(maxY, blk.getY2());
    }

    if (maxX == 0 || maxY == 0 || areaNorm == 0 || wireNorm == 0 || 
        aspectNorm == 0 || outlineXNorm == 0 || outlineYNorm == 0) 
    {
        cout << "maxX: " << maxX << ", maxY: " << maxY << endl;
        cout << "areaNorm: " << areaNorm << ", wireNorm: " << wireNorm << endl;
        cout << "aspectNorm: " << aspectNorm << ", outlineXNorm: " << outlineXNorm << endl;
        cout << "outlineYNorm: " << outlineYNorm << endl;
        return 1e9;
    }
        

    // Calculate aspect ratio term
    double R = static_cast<double>(maxY) / maxX;
    double R_star = static_cast<double>(_outlineHeight) / _outlineWidth;
    double aspectPenalty = std::pow(R - R_star, 2.0);

    // Calculate outline constraint penalties
    double outlineXPenalty = std::max(0.0, static_cast<double>(maxX) - _outlineWidth);
    double outlineYPenalty = std::max(0.0, static_cast<double>(maxY) - _outlineHeight);
    
    // Normalize all terms
    double normA = A / areaNorm;
    double normW = W / wireNorm;
    double normR = aspectPenalty / aspectNorm;
    double normX = outlineXPenalty / outlineXNorm;
    double normY = outlineYPenalty / outlineYNorm;
    
    // Add outline penalties to cost function
    double outlineXPenaltyWeight = 6;
    double outlineYPenaltyWeight = 6;
    double cost = alpha * normA + (1-alpha) * normW + beta * normR + 
                 outlineXPenaltyWeight * normX + outlineYPenaltyWeight * normY;

    // Log the metrics
    // logCostMetrics(iteration++, normA, normW, normR, normX, normY, cost);

    return cost;
}
void Floorplanner::logCostMetrics(int iteration, double normA, double normW, 
    double normR, double normX, double normY, double cost) {
    static bool firstCall = true;
    static std::ofstream logFile("cost_metrics.txt");
    
    if (firstCall) {
        logFile << "Iteration,NormArea,NormWL,NormAspect,NormOutlineX,NormOutlineY,TotalCost\n";
        firstCall = false;
    }
    
    logFile << iteration << "," << normA << "," << normW << "," 
            << normR << "," << normX << "," << normY << "," << cost << "\n";
}


// void Floorplanner::computeNormalizationFactors(double& areaNorm, double& wlNorm, double& aspectNorm, int sampleSize ) {
//     double totalArea = 0, totalWL = 0, totalAspect = 0;

//     for (int i = 0; i < 100; ++i) {
//         _tree->rotateRandom();
//         _tree->deleteAndInsert();
//         _tree->swapRandomNodes();
//     }

//     for (int i = 0; i < sampleSize; ++i) {
//         _tree->rotateRandom();
//         _tree->deleteAndInsert();
//         _tree->swapRandomNodes();
//         _tree->pack();

//         totalArea += computeArea();
//         totalWL += computeWirelength();

//         size_t maxX = 0, maxY = 0;
//         for (Block& blk : _block_array) {
//             maxX = std::max(maxX, blk.getX2());
//             maxY = std::max(maxY, blk.getY2());
//         }
//         double aspect = static_cast<double>(maxY) / maxX;
//         totalAspect += std::pow(aspect - static_cast<double>(_outlineHeight) / _outlineWidth, 2);
//     }

//     areaNorm = totalArea / sampleSize;
//     wlNorm = totalWL / sampleSize;
//     aspectNorm = totalAspect / sampleSize;
// }

void Floorplanner::computeNormalizationFactors(double& areaNorm, double& wlNorm, 
    double& aspectNorm, double& outlineXNorm, double& outlineYNorm, int sampleSize) {
    
    double totalArea = 0, totalWL = 0, totalAspect = 0;
    double totalOutlineX = 0.001, totalOutlineY = 0.001;
    double R_target = static_cast<double>(_outlineHeight) / _outlineWidth;

    // Initial random perturbations
    for (int i = 0; i < 100; ++i) {
        _tree->rotateRandom();
        _tree->deleteAndInsert();
        _tree->swapRandomNodes();
    }

    // Sample solutions
    for (int i = 0; i < sampleSize; ++i) {
        _tree->rotateRandom();
        _tree->deleteAndInsert();
        _tree->swapRandomNodes();
        _tree->pack();

        totalArea += computeArea();
        totalWL += computeWirelength();

        size_t maxX = 0, maxY = 0;
        for (Block& blk : _block_array) {
            maxX = std::max(maxX, blk.getX2());
            maxY = std::max(maxY, blk.getY2());
        }

        // Calculate aspect ratio penalty
        double R = static_cast<double>(maxY) / maxX;
        double aspectPenalty = std::pow(R - R_target, 2.0);
        totalAspect += aspectPenalty;

        // Calculate outline constraint penalties
        double outlineXPenalty = std::max(0.0, static_cast<double>(maxX) - _outlineWidth);
        // cout << "outlineXPenalty: " << outlineXPenalty << endl;
        double outlineYPenalty = std::max(0.0, static_cast<double>(maxY) - _outlineHeight);
        totalOutlineX += outlineXPenalty;
        totalOutlineY += outlineYPenalty;
    }

    // Compute average values as normalization factors
    areaNorm = totalArea / sampleSize;
    wlNorm = totalWL / sampleSize;
    aspectNorm = totalAspect / sampleSize;
    outlineXNorm = totalOutlineX / sampleSize;
    outlineYNorm = totalOutlineY / sampleSize;
}






// void Floorplanner::simulatedAnnealing() {
//     // Initialize temperature and cooling parameters
//     double T = 1000.0;       // Initial temperature 
//     const double T_min = 0.1;
//     const double cooling_rate = 0.99;
//     const int iterations = 1000; // Iterations per temperature


//     _beta = 5;
//     // Compute normalization factors
//     double areaNorm, wireNorm, aspectNorm;
//     computeNormalizationFactors(areaNorm, wireNorm, aspectNorm, 100);

//     _alpha = 0.5; // Set alpha to a fixed value for simplicity
//     cout << "alpha: " << _alpha << ", beta: " << _beta << endl;
//     // Evaluate initial solution
//     _tree->pack();
//     double prevCost = computeCost(_alpha, _beta, areaNorm, wireNorm, aspectNorm);
//     double bestCost = prevCost;

//     Tree bestSolution = *_tree;  // store best solution by value

//     while (T > T_min) {
//         cout << "iterations: " << iterations << "Temperature: " << T << ", Cost: " << prevCost << endl;


//         for (int i = 0; i < iterations; ++i) {
//             Tree backup = *_tree;                       // Save current state
//             double oldCost = prevCost;

//             // Perform random perturbation
//             // int op = rand() % 3;
//             // switch (op) {
//             //     case 0: _tree->rotateRandom(); break;
//             //     case 1: _tree->swapRandomNodes(); break;
//             //     case 2: _tree->deleteAndInsert(); break;
//             // }
//             double r = static_cast<double>(rand()) / RAND_MAX;  // Random number between 0 and 1

//             // Specify probabilities for each operation
//             // rotate: 40%, swap: 35%, delete-insert: 25%
//             if (r < 0.250) {
//                 _tree->rotateRandom();
//             } else if (r < 0.75) {  // 0.40 + 0.35 = 0.75
//                 _tree->swapRandomNodes();
//             } else {  // remaining 25%
//                 _tree->deleteAndInsert();
//             }

//             _tree->pack();                              // Repack after perturbation
//             double newCost = computeCost(_alpha, _beta, areaNorm, wireNorm, aspectNorm);
//             double deltaCost = newCost - oldCost;

//             double probability = exp(-deltaCost / T);
//             if (deltaCost < 0 || (double)rand() / RAND_MAX < probability) {
//                 prevCost = newCost;                     // Accept new state
//                 if (newCost < bestCost) {
//                     bestCost = newCost;
//                     bestSolution = *_tree;              // Update best solution
//                 }
//             } else {
//                 *_tree = backup;                        // Revert state
//             }
//         }

//         T *= cooling_rate;
//         // _beta = initial_beta * (T / initial_T);  // adaptively reduce beta

//     }

//     // Restore best solution
//     *_tree = bestSolution;
//     _tree->pack();
//     _tree->exportToFile("output.txt", _outlineWidth, _outlineHeight); // Export final solution

//     cout << "==== Simulated Annealing Done ====" << endl;
//     cout << "Best Cost: " << bestCost << endl;
//     cout << "Final Area: " << computeArea() << endl;
//     cout << "Final Wirelength: " << computeWirelength() << endl;
// }





// void Floorplanner::simulatedAnnealing() {
//     // Initialize temperature and cooling parameters
//     double T = 1000.0;       // Initial temperature 
//     const double T_min = 0.1;
//     const double cooling_rate = 0.99;
//     const int iterations = 1000; // Iterations per temperature


//     _beta = 5;
//     // Compute normalization factors
//     double areaNorm, wireNorm, aspectNorm;
//     computeNormalizationFactors(areaNorm, wireNorm, aspectNorm, 100);

//     _alpha = 0.5; // Set alpha to a fixed value for simplicity
//     cout << "alpha: " << _alpha << ", beta: " << _beta << endl;
//     // Evaluate initial solution
//     _tree->pack();
//     double prevCost = computeCost(_alpha, _beta, areaNorm, wireNorm, aspectNorm);
//     double bestCost = prevCost;

//     Tree bestSolution = *_tree;  // store best solution by value

//     while (T > T_min) {
//         cout << "iterations: " << iterations << "Temperature: " << T << ", Cost: " << prevCost << endl;


//         for (int i = 0; i < iterations; ++i) {
//             Tree backup = *_tree;                       
//             double oldCost = prevCost;

//             // Perform random perturbation
//             // int op = rand() % 3;
//             // switch (op) {
//             //     case 0: _tree->rotateRandom(); break;
//             //     case 1: _tree->swapRandomNodes(); break;
//             //     case 2: _tree->deleteAndInsert(); break;
//             // }
//             double r = static_cast<double>(rand()) / RAND_MAX;  

//             if (r < 0.250) {
//                 _tree->rotateRandom();
//             } else if (r < 0.75) {  
//                 _tree->swapRandomNodes();
//             } else {  
//                 _tree->deleteAndInsert();
//             }

//             _tree->pack();                              // Repack after perturbation
//             double newCost = computeCost(_alpha, _beta, areaNorm, wireNorm, aspectNorm);
//             double deltaCost = newCost - oldCost;

//             double probability = exp(-deltaCost / T);
//             if (deltaCost < 0 || (double)rand() / RAND_MAX < probability) 
//             // accept the solution
//             {
//                 prevCost = newCost;                     // Accept new state
//                 if (newCost < bestCost) {
//                     bestCost = newCost;
//                     bestSolution = *_tree;              // Update best solution
//                 }
//             } else 
//             // reject the solution
//             {
//                 *_tree = backup;                        // Revert state
//             }
//         }

//         T *= cooling_rate;

//     }

//     // Restore best solution
//     *_tree = bestSolution;
//     _tree->pack();
//     _tree->exportToFile("output.txt", _outlineWidth, _outlineHeight); // Export final solution

//     cout << "==== Simulated Annealing Done ====" << endl;
//     cout << "Best Cost: " << bestCost << endl;
//     cout << "Final Area: " << computeArea() << endl;
//     cout << "Final Wirelength: " << computeWirelength() << endl;
// }]




void Floorplanner::simulatedAnnealing() {
    // Initialize parameters
    double T = 100000.0;
    const double T_min = 0.1;
    const double cooling_rate = 0.99;
    const int iterations = 180;

    // Adaptive beta parameters
    _beta = 4.0;  // Start with high beta to enforce aspect ratio
    const double beta_min = 0.5;  // Minimum beta value
    const double aspect_threshold = 0.0001;  // Threshold for good aspect ratio
    const double beta_decay = 0.99;  // Beta reduction rate

    // Compute normalization factors
    double areaNorm, wireNorm, aspectNorm, outlineXNorm, outlineYNorm;
    computeNormalizationFactors(areaNorm, wireNorm, aspectNorm, outlineXNorm, outlineYNorm, 100);

    // _alpha = 0.9;
    _tree->pack();
    double prevCost = computeCost(_alpha, _beta, areaNorm, wireNorm, aspectNorm, outlineXNorm, outlineYNorm);
    double bestCost = prevCost;
    Tree bestSolution = *_tree;

    while (T > T_min) {
        // cout << "Temperature: " << T << ", Beta: " << _beta << ", Cost: " << prevCost << endl;

        for (int i = 0; i < iterations; ++i) {
            Tree backup = *_tree;
            double oldCost = prevCost;

            // Perform perturbation
            double r = static_cast<double>(rand()) / RAND_MAX;
            if (r < 0.15) {
                _tree->rotateRandom();
            } else if (r < 0.65) {
                _tree->swapRandomNodes();
            } else {
                _tree->deleteAndInsert();
            }


            _tree->pack();
            
            // Calculate individual cost terms
            double A = computeArea();
            double W = computeWirelength();
            size_t maxX = 0, maxY = 0;
            for (Block& blk : _block_array) {
                maxX = std::max(maxX, blk.getX2());
                maxY = std::max(maxY, blk.getY2());
            }
            double R = static_cast<double>(maxY) / maxX;
            double R_star = static_cast<double>(_outlineHeight) / _outlineWidth;
            double aspectPenalty = std::pow(R - R_star, 2.0);
            double normR = aspectPenalty / aspectNorm;

            // Adapt beta if aspect ratio is good enough
            if (normR < aspect_threshold && _beta > beta_min) {
                _beta *= beta_decay;
                _beta = std::max(_beta, beta_min);
            }

            double newCost = computeCost(_alpha, _beta, areaNorm, wireNorm, aspectNorm, outlineXNorm, outlineYNorm);
            double deltaCost = newCost - oldCost;

            double probability = exp(-deltaCost / T)/1000;
            if (deltaCost < 0 || (double)rand() / RAND_MAX < probability) {
                prevCost = newCost;
                if (newCost < bestCost) {
                    bestCost = newCost;
                    bestSolution = *_tree;
                }
            } else {
                *_tree = backup;
                prevCost = oldCost;
            }
        }

        T *= cooling_rate;
    }

    // Restore best solution
    *_tree = bestSolution;
    _tree->pack();
    // _tree->exportToFile("output.txt", _outlineWidth, _outlineHeight);
    outputArea = computeArea();
    outputWirelength = computeWirelength();
    outputCost = _alpha * outputArea + (1 - _alpha) * outputWirelength;

    cout << "==== Simulated Annealing Done ====" << endl;
    cout << "Best Cost: " << bestCost << endl;
    cout << "output cost: " << outputCost << endl;
    cout << "Final Area: " << outputArea << endl;
    cout << "Final Wirelength: " << outputWirelength << endl;
}