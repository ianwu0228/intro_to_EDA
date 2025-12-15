#ifndef GLOBALPLACER_H
#define GLOBALPLACER_H

#include <string>
#include <vector>
#include <map>
#include <tuple>
#include <gurobi_c++.h> // 包含 Gurobi 標頭檔
#include "Module.h"     // 包含數據結構

using namespace std;

// 全局放置器類別
class GlobalPlacer {
public:
    FloorplanData& data; // 引用 FloorplanData 

    // 建構子
    GlobalPlacer(FloorplanData& floorplan_data) : data(floorplan_data) {}

    // 主要放置函數
    void place();

private:
    // Gurobi 變數映射 (用於快速查找)
    map<string, GRBVar> x_vars; // 模組中心 X 座標變數
    map<string, GRBVar> y_vars; // 模組中心 Y 座標變數
    
    // Gurobi 距離變數映射 (用於 HPWL 線性化: |c_i - c_j|)
    // 鍵值: {ModuleA, ModuleB} (按字母順序排列)
    map<pair<string, string>, GRBVar> dx_vars; 
    map<pair<string, string>, GRBVar> dy_vars; 

    // 執行 ILP 求解
    void runILPPlacement(GRBModel& model);

    // 實作線性重疊懲罰項
    void addOverlapPenalty(GRBModel& model, GRBLinExpr& obj_Penalty);
    
    // 實作 HPWL 線性化約束
    void addHPWLConstraints(GRBModel& model, GRBLinExpr& obj_HPWL);

    // 儲存結果到 FloorplanData
    void storeResults(GRBModel& model);
};

#endif // GLOBALPLACER_H