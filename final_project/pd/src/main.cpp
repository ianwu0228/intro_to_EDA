#include <iostream>
#include <fstream>
#include <vector>
#include <ctime>    
#include <cstdlib>   
#include <chrono>

#include "floorplanner.h"
#include "tree.h"
using namespace std;

int main(int argc, char** argv)
{
    srand(static_cast<unsigned int>(time(0)));
    fstream input_blk, input_net, output;
    double alpha;


    // start timing 
    auto start_time = chrono::high_resolution_clock::now();


    if (argc == 5) {
        alpha = stod(argv[1]);
        input_blk.open(argv[2], ios::in);
        input_net.open(argv[3], ios::in);
        output.open(argv[4], ios::out);
        if (!input_blk) {
            cerr << "Cannot open the input file \"" << argv[2]
                 << "\". The program will be terminated..." << endl;
            exit(1);
        }
        if (!input_net) {
            cerr << "Cannot open the input file \"" << argv[3]
                 << "\". The program will be terminated..." << endl;
            exit(1);
        }
        if (!output) {
            cerr << "Cannot open the output file \"" << argv[4]
                 << "\". The program will be terminated..." << endl;
            exit(1);
        }
    }
    else {
        cerr << "Usage: ./Floorplanner <alpha> <input block file> " <<
                "<input net file> <output file>" << endl;
        exit(1);
    }

    Floorplanner* fp = new Floorplanner(input_blk, input_net, alpha);
    fp->floorplan();



    // End timing
    auto end_time = chrono::high_resolution_clock::now();
    auto duration = chrono::duration_cast<chrono::milliseconds>(end_time - start_time);
    cout << "Time taken: " << duration.count()*0.001 << " s" << endl;
    fp->computeMaxX();
    fp->computeMaxY();
    fp->_tree->exportToFile(output, fp->getOutlineWidth(), fp->getOutlineHeight(), duration, fp->getOutputCost(),  fp->getOutputWirelength(), fp->getOutputArea(), fp->_maxX, fp->_maxY);
    cout << "alpha: " << fp->_alpha << endl;

    if(fp->_maxX < fp->getOutlineWidth() && fp->_maxY < fp->getOutlineHeight())
    {
        cout << "Floorplan is feasible." << endl;
    }
    else
    {
        cout << "Floorplan is infeasible." << endl;
    }
    cout << "=============== Floorplan completed. ===============" << endl;

//    fstream output_visual;
//    output_visual.open("visualize.txt", ios::out);
//    fp->_tree->exportToFile_visualize(output_visual, fp->getOutlineWidth(), fp->getOutlineHeight(), duration, fp->getOutputCost(),  fp->getOutputWirelength(), fp->getOutputArea(), fp->_maxX, fp->_maxY);


    return 0;
}


// int main(int argc, char** argv)
// {

//     srand(static_cast<unsigned int>(time(0)));


//     fstream input_blk, input_net, output;
//     input_blk.open("./input_pa2/ami49.block", ios::in);
//     input_net.open("./input_pa2/ami49.nets", ios::in);
//     Floorplanner* fp = new Floorplanner(input_blk, input_net, 0.5);
//     cout << "start parsing block file" << endl;
//     fp->parseBlockFile(input_blk);
//     fp->parseNetFile(input_net);

//     fp->_tree->buildInitial();
//     fp->_tree->pack();
//     fp->_tree->exportToFile("initial.txt", fp->_outlineWidth, fp->_outlineHeight);
//     Node* root = fp->_tree->getRoot();
//     // for(int i = 0; i < 10000; ++i)
//     // {
//     //     fp->_tree->rotateRandom();
//     //     fp->_tree->deleteAndInsert();
//     //     fp->_tree->swapRandomNodes();
//     // }


//     fp->simulatedAnnealing();





//     cout << "============= done =============" << endl;

    


//     return 0;
// }