#ifndef MODULE_H
#define MODULE_H

#include <string>
#include <vector>
#include <map>
#include <cmath>
#include <algorithm>
#include <utility> // for std::pair

using namespace std;

// 點結構
struct Point {
    int x;
    int y;
};

// 矩形結構
struct Rect {
    int x_min;
    int y_min;
    int x_max;
    int y_max;
};

// 模組結構
struct Module {
    string name;
    bool is_soft;
    long long min_area; // 使用 long long 處理大面積
    
    // Global Part 資訊
    int s_len; // 簡化後的邊長 (si)
    Point global_center; // Global Placement 輸出的中心座標
    double global_score; // 用於排序的分數
    
    // Local Part 資訊
    Rect current_bbox; // 當前 BBox (x_min, y_min, x_max, y_max)
    vector<Point> corners; // 最終形狀的轉角座標
    vector<Point> occupied_cells; // Local Part 追蹤佔據的網格座標
    
    // Fixed Module 特有資訊
    int fixed_x;
    int fixed_y;
    int fixed_w;
    int fixed_h;
};

// 儲存所有模組和連線
struct FloorplanData {
    int chip_w;
    int chip_h;
    vector<Module> soft_modules;
    vector<Module> fixed_modules;
    map<pair<string, string>, int> connections; // 儲存 (ModuleA, ModuleB) -> NetCount
    
    // 函數原型
    void readInput(const string& filename);
    const Module* getModuleByName(const string& name) const;
};

// 函數：計算曼哈頓距離
double manhattanDistance(Point p1, Point p2);

#endif // MODULE_H