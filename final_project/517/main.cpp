#include <iostream>
#include <string>
#include <cstdlib>
#include "Module.h"
#include "GlobalPlacer.h"
#include "LocalRefiner.h"

using namespace std;

int main(int argc, char *argv[]) {
    // 檢查命令行參數
    if (argc != 3) {
        cerr << "Usage: " << argv[0] << " [input_filename] [output_filename]" << endl;

        cerr << "Example: " << "./cadd0000 case01-input.txt case01-output.txt" << endl; 
        return 1;
    }
    
    string input_filename = argv[1];
    string output_filename = argv[2];
    
    cout << "======================================================" << endl;
    cout << "        ICCAD 2023 Problem D Floorplanner Start" << endl;
    cout << "======================================================" << endl;
    cout << "Input File: " << input_filename << endl;
    cout << "Output File: " << output_filename << endl;
    
    // 1. 讀取輸入檔案
    FloorplanData data;
    data.readInput(input_filename);
    
    // 2. Global Part: 建立初始拓樸 (使用 Gurobi)
    GlobalPlacer placer(data);
    placer.place();
    
    // 3. Local Part: 形狀生成與細部調整
    LocalRefiner refiner(data);
    refiner.refineAndOutput(output_filename);
    
    cout << "======================================================" << endl;
    cout << "Floorplanning Finished. Output written to " << output_filename << endl;
    cout << "======================================================" << endl;

    return 0;
}