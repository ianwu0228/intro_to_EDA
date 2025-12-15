#ifndef LOCALREFINER_H
#define LOCALREFINER_H

#include "Module.h"
#include <vector>
#include <map>
#include <iostream>
#include <cmath>
#include <algorithm>
#include <fstream>
#include <sstream>

using namespace std;

// 定義延伸方向
enum Direction {
    LEFT = 0,
    RIGHT = 1,
    UP = 2,
    DOWN = 3
};

class LocalRefiner {
private:
    FloorplanData& data;
    // 網格: [y][x] 儲存 Module ID (0 代表未佔據)
    vector<vector<int>> grid; 
    
    // 1. 初始化網格
    void initializeGrid();
    
    // 2. 判斷是否所有 Soft Modules 都已滿足最小面積
    bool isAreaConstraintSatisfied();

    // 3. 步驟 3: 選擇最佳延伸方向 (d1 -> d4)
    vector<Direction> getExtensionOrdering(const Module& m);
    
    // 4. 步驟 5/6: 嘗試向特定方向延伸 1 格
    bool tryExtend(Module& m, Direction dir);
    
    // 5. 步驟 7: 執行 Round-Robin 延伸
    void runExtensionRounds();

    // 6. 最終計算 HPWL
    double calculateFinalHPWL() const;

    // 7. 最終輸出
    void writeOutput(const string& output_filename, double hpwl);

    // 8. 輔助函數：從佔據的格子生成順時針轉角列表 (複雜幾何操作)
    vector<Point> generateCorners(const Module& m) const;

public:
    LocalRefiner(FloorplanData& fd) : data(fd) {}
    void refineAndOutput(const string& output_filename);
};

#endif // LOCALREFINER_H