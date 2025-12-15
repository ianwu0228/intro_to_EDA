#include "Module.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <cmath> // 包含 ceil 和 sqrt

using namespace std;

// 函數：計算曼哈頓距離 (HPWL 需要)
double manhattanDistance(Point p1, Point p2) {
    // 曼哈頓距離: |x1 - x2| + |y1 - y2|
    return abs(p1.x - p2.x) + abs(p1.y - p2.y);
}

// 函數：根據名稱查找模組
const Module* FloorplanData::getModuleByName(const string& name) const {
    // 查找 Soft Modules
    for (const auto& m : soft_modules) {
        if (m.name == name) return &m;
    }
    // 查找 Fixed Modules
    for (const auto& m : fixed_modules) {
        if (m.name == name) return &m;
    }
    return nullptr;
}

// 函數：讀取輸入檔案
void FloorplanData::readInput(const string& filename) {
    ifstream file(filename);
    string line;
    
    if (!file.is_open()) {
        cerr << "Error: Cannot open input file " << filename << endl;
        return;
    }
    
    cout << "--- Starting Input File Parsing ---" << endl;

    // 讀取 CHIP, SOFTMODULE, FIXEDMODULE, CONNECTION
    while (getline(file, line)) {
        stringstream ss(line);
        string keyword;
        ss >> keyword;
        
        if (keyword == "CHIP") {
            // CHIP chip_width chip_height
            ss >> chip_w >> chip_h;
        } else if (keyword == "SOFTMODULE") {
            // SOFTMODULE number_of_soft_modules
            int num;
            ss >> num;
            for (int i = 0; i < num; ++i) {
                if (getline(file, line)) {
                    stringstream ss_m(line);
                    Module m;
                    m.is_soft = true;
                    // soft_module_name minimum_area
                    ss_m >> m.name >> m.min_area;
                    
                    // Global Part 幾何簡化 (si = ceil(sqrt(Ai)))
                    // 所有 soft modules 視為正方形
                    // si = ceil(sqrt(Ai))
                    m.s_len = ceil(sqrt(m.min_area));
                    soft_modules.push_back(m);
                }
            }
        } else if (keyword == "FIXEDMODULE") {
            // FIXEDMODULE number_of_fixed_modules
            int num;
            ss >> num;
            for (int i = 0; i < num; ++i) {
                if (getline(file, line)) {
                    stringstream ss_m(line);
                    Module m;
                    m.is_soft = false;
                    // fixed_module_name x_coord y_coord module_width module_height
                    ss_m >> m.name >> m.fixed_x >> m.fixed_y >> m.fixed_w >> m.fixed_h;
                    m.min_area = (long long)m.fixed_w * m.fixed_h;
                    
                    // 計算 Fixed Module 的中心和簡化邊長 (用於 HPWL 和 Penalty)
                    // 曼哈頓距離計算使用最小矩形的中心
                    m.global_center = {m.fixed_x + m.fixed_w / 2, m.fixed_y + m.fixed_h / 2};
                    // sf = ceil(sqrt(wf * hf))
                    m.s_len = ceil(sqrt(m.min_area)); 
                    // BBox 範圍
                    m.current_bbox = {m.fixed_x, m.fixed_y, m.fixed_x + m.fixed_w, m.fixed_y + m.fixed_h};
                    fixed_modules.push_back(m);
                }
            }
        } else if (keyword == "CONNECTION") {
            // CONNECTION number_of_connections
            int num;
            ss >> num;
            for (int i = 0; i < num; ++i) {
                if (getline(file, line)) {
                    stringstream ss_c(line);
                    string name1, name2;
                    int net_count;
                    // module_name1 module_name2 number_of_2_pin_nets
                    ss_c >> name1 >> name2 >> net_count;
                    
                    // 以字典序儲存 pair 以確保唯一性
                    if (name1 > name2) swap(name1, name2);
                    connections[{name1, name2}] = net_count;
                }
            }
        }
    }
    
    // 簡化計算完成
    cout << "Input reading finished. Chip: " << chip_w << "x" << chip_h << endl;
    cout << "--- Input File Parsing Finished ---" << endl;
}