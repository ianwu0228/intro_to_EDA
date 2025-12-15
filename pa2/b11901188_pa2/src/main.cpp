#include "gurobi_c++.h"
#include "floorplanner.h"
#include <iostream>
#include <cstdlib> // for std::system
#include <cstdio>  // for std::remove
using namespace std;

int main(int argc, char **argv)
{

    if (argc != 4)
    {
        std::cerr << "Usage: " << argv[0] << " <inputFile> <specFile> <outputFile>" << std::endl;
        return 1;
    }
    Floorplanner fp_;
    fp_.initialize(argv[1]);
    fp_.setSpec(Spec(argv[2]));
    if (fp_.getProblemType() == 0)
    {
        fp_.solve();
        fp_.validityCheck(); // you can comment out this function
        fp_.writeOutput(argv[3]);
        return 0;
    }
    else
    {
        // temp files for converted format
        std::string blockFile = "tmp.block";
        std::string netsFile = "tmp.nets";

        // 1) run format_exchange:
        //    ./bin/format_exchange <inputFile> <specFile> <output.block> <output.nets>
        std::string cmd1 = "./bin/format_exchange " +
                           std::string(argv[2]) + " " +
                           std::string(argv[1]) + " " +
                           blockFile + " " + netsFile;

        std::cout << "[INFO] Running: " << cmd1 << std::endl;
        int ret1 = std::system(cmd1.c_str());
        if (ret1 != 0)
        {
            std::cerr << "Error: format_exchange failed with code " << ret1 << std::endl;
            return ret1;
        }

        // 2) run b_fp:
        //    ./bin/b_fp 1 <output.block> <output.nets> <output_file>
        std::string cmd2 = "./bin/b_fp 1 " +
                           blockFile + " " + netsFile + " " +
                           std::string(argv[3]);

        std::cout << "[INFO] Running: " << cmd2 << std::endl;
        int ret2 = std::system(cmd2.c_str());
        if (ret2 != 0)
        {
            std::cerr << "Error: b_fp failed with code " << ret2 << std::endl;
            return ret2;
        }

        // 3) optional: clean temp files
        std::remove(blockFile.c_str());
        std::remove(netsFile.c_str());

        return 0;
    }
}
