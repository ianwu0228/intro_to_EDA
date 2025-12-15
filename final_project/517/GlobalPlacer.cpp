#include "GlobalPlacer.h"
#include <iostream>
#include <numeric>

using namespace std;

// ----------------------------------------------------------------------
// 輔助函數: 取得模組變數的邊長
// ----------------------------------------------------------------------
double getModuleSize(const string& name, const FloorplanData& data) {
    for (const auto& m : data.soft_modules) {
        if (m.name == name) return m.s_len;
    }
    // 必須是 Fixed Module
    for (const auto& m : data.fixed_modules) {
        if (m.name == name) return m.s_len; 
    }
    return 0.0; // 應永遠不會發生
}

// ----------------------------------------------------------------------
// 放置主函數
// ----------------------------------------------------------------------
void GlobalPlacer::place() {
    cout << "--- Starting Global Placement (ILP with Gurobi) ---" << endl;
    
    try {
        GRBEnv env = GRBEnv(true);
        env.set("LogFile", "global_placement.log");
        env.start();
        
        GRBModel model = GRBModel(env);
        model.set(GRB_IntParam_OutputFlag, 1); // 輸出 Gurobi 日誌
        model.set(GRB_DoubleParam_TimeLimit, 120.0); // 設定運行時間限制 (30s)

        runILPPlacement(model);
        
        // 求解模型
        model.optimize();

        if (model.get(GRB_IntAttr_Status) == GRB_OPTIMAL) {
            cout << "Optimization complete. Optimal HPWL found." << endl;
            storeResults(model);
        } else if (model.get(GRB_IntAttr_Status) == GRB_TIME_LIMIT) {
             cout << "Optimization stopped due to time limit. Using best solution found." << endl;
             // 嘗試獲取最優解
             if (model.get(GRB_IntAttr_SolCount) > 0) {
                 storeResults(model);
             }
        } else {
            cout << "Optimization failed. Status code: " << model.get(GRB_IntAttr_Status) << endl;
        }
        
    } catch (GRBException e) {
        cout << "Error code = " << e.getErrorCode() << endl;
        cout << e.getMessage() << endl;
    } catch (...) {
        cout << "Exception during optimization" << endl;
    }
    
    cout << "--- Global Placement Finished ---" << endl;
}

// ----------------------------------------------------------------------
// 執行 ILP 求解器
// ----------------------------------------------------------------------
void GlobalPlacer::runILPPlacement(GRBModel& model) {
    
    // 1. 變數定義 (Center Variables)
    double chip_w = data.chip_w;
    double chip_h = data.chip_h;
    
    // 定義 Soft Module 的中心點變數 (X/Y)
    for (const auto& m : data.soft_modules) {
        // Center_X 範圍: [s_len/2, ChipW - s_len/2]
        double min_x = m.s_len / 2.0;
        double max_x = chip_w - m.s_len / 2.0;
        
        x_vars[m.name] = model.addVar(min_x, max_x, 0.0, GRB_CONTINUOUS, "X_" + m.name);
        y_vars[m.name] = model.addVar(min_x, max_x, 0.0, GRB_CONTINUOUS, "Y_" + m.name);
    }
    
    // Fixed Module 的中心點是固定的 (僅用於連線計算，不需要變數)
    // 但為了統一 HPWL 計算，我們將 Fixed Module 的中心點作為常數項處理。

    // 2. 變數定義 (Distance Variables for HPWL)
    // 遍歷所有模組對，計算連線數大於 0 的 pairs
    for (size_t i = 0; i < data.soft_modules.size(); ++i) {
        const Module& m_i = data.soft_modules[i];

        // Soft-Soft Pairs
        for (size_t j = i + 1; j < data.soft_modules.size(); ++j) {
            const Module& m_j = data.soft_modules[j];

            // 確定連線數量 (如果不存在，則為 0)
            string name1 = m_i.name, name2 = m_j.name;
            if (name1 > name2) swap(name1, name2);
            pair<string, string> module_pair = {name1, name2};
            
            if (data.connections.count(module_pair) > 0) {
                // 定義距離變數 dx, dy
                GRBVar dx = model.addVar(0.0, chip_w, 0.0, GRB_CONTINUOUS, "DX_" + name1 + "_" + name2);
                GRBVar dy = model.addVar(0.0, chip_h, 0.0, GRB_CONTINUOUS, "DY_" + name1 + "_" + name2);
                
                dx_vars[module_pair] = dx;
                dy_vars[module_pair] = dy;
            }
        }

        // Soft-Fixed Pairs
        for (const auto& m_f : data.fixed_modules) {
            string name1 = m_i.name, name2 = m_f.name;
            if (name1 > name2) swap(name1, name2);
            pair<string, string> module_pair = {name1, name2};

            if (data.connections.count(module_pair) > 0) {
                 // 定義距離變數 dx, dy
                GRBVar dx = model.addVar(0.0, chip_w, 0.0, GRB_CONTINUOUS, "DX_" + name1 + "_" + name2);
                GRBVar dy = model.addVar(0.0, chip_h, 0.0, GRB_CONTINUOUS, "DY_" + name1 + "_" + name2);
                
                dx_vars[module_pair] = dx;
                dy_vars[module_pair] = dy;
            }
        }
    }
    
    // 3. 加入 HPWL 約束
    GRBLinExpr obj_HPWL = 0;
    addHPWLConstraints(model, obj_HPWL);
    
    // 4. 加入重疊懲罰項
    GRBLinExpr obj_Penalty = 0;
    addOverlapPenalty(model, obj_Penalty);
    
    // 5. 設定目標函數: Minimize HPWL + Penalty
    model.setObjective(obj_HPWL + obj_Penalty, GRB_MINIMIZE);

    // 6. 設定 Gurobi 參數
    model.set(GRB_IntParam_LogToConsole, 0); // 關閉控制台輸出 (可根據需要調整)
}

// ----------------------------------------------------------------------
// 實作 HPWL 線性化約束
// ----------------------------------------------------------------------
void GlobalPlacer::addHPWLConstraints(GRBModel& model, GRBLinExpr& obj_HPWL) {
    cout << "  - Adding HPWL Constraints..." << endl;
    
    for (const auto& conn : data.connections) {
        const pair<string, string>& module_pair = conn.first;
        int net_count = conn.second;
        
        string name1 = module_pair.first;
        string name2 = module_pair.second;
        
        // 查找距離變數
        if (dx_vars.count(module_pair) == 0) continue;
        GRBVar dx = dx_vars.at(module_pair);
        GRBVar dy = dy_vars.at(module_pair);

        // A. 處理 Soft-Soft 連線
        if (x_vars.count(name1) && x_vars.count(name2)) {
            GRBVar x1 = x_vars.at(name1);
            GRBVar x2 = x_vars.at(name2);
            GRBVar y1 = y_vars.at(name1);
            GRBVar y2 = y_vars.at(name2);

            // |x1 - x2| <= dx    ->  dx >= x1 - x2  and  dx >= x2 - x1
            model.addConstr(dx >= x1 - x2, "DX_P_" + name1 + "_" + name2);
            model.addConstr(dx >= x2 - x1, "DX_N_" + name1 + "_" + name2);
            model.addConstr(dy >= y1 - y2, "DY_P_" + name1 + "_" + name2);
            model.addConstr(dy >= y2 - y1, "DY_N_" + name1 + "_" + name2);
        }
        // B. 處理 Soft-Fixed 連線
        else if (x_vars.count(name1) || x_vars.count(name2)) {
            // 確定 Soft Module 和 Fixed Module 的變數/中心
            GRBVar x_soft, y_soft;
            double x_fixed, y_fixed;
            string soft_name, fixed_name;

            if (x_vars.count(name1)) { // name1 是 Soft
                x_soft = x_vars.at(name1);
                y_soft = y_vars.at(name1);
                soft_name = name1;
                fixed_name = name2;
            } else { // name2 是 Soft
                x_soft = x_vars.at(name2);
                y_soft = y_vars.at(name2);
                soft_name = name2;
                fixed_name = name1;
            }
            
            // 查找 Fixed Module 的中心座標 (從 FloorplanData 中獲取)
            const Module* m_f = data.getModuleByName(fixed_name);
            if (!m_f) continue;
            
            x_fixed = m_f->fixed_x + m_f->fixed_w / 2.0;
            y_fixed = m_f->fixed_y + m_f->fixed_h / 2.0;

            // |x_soft - x_fixed| <= dx
            model.addConstr(dx >= x_soft - x_fixed, "DX_SF_P_" + soft_name + "_" + fixed_name);
            model.addConstr(dx >= x_fixed - x_soft, "DX_SF_N_" + soft_name + "_" + fixed_name);
            model.addConstr(dy >= y_soft - y_fixed, "DY_SF_P_" + soft_name + "_" + fixed_name);
            model.addConstr(dy >= y_fixed - y_soft, "DY_SF_N_" + soft_name + "_" + fixed_name);
        }
        // C. Fixed-Fixed 連線 (不需要變數，距離是常數)
        // 此處忽略，因為它們不會影響 HPWL 最小化目標。

        // 加入 HPWL 目標函數 (HPWL = Sum(NetCount * (dx + dy)))
        obj_HPWL += net_count * (dx + dy);
    }
}

// ----------------------------------------------------------------------
// 實作線性重疊懲罰項
// Penalty = 2 * (Avg_Connection) * Sum(Overlap_x + Overlap_y)
// ----------------------------------------------------------------------
void GlobalPlacer::addOverlapPenalty(GRBModel& model, GRBLinExpr& obj_Penalty) {
    
    // 1. 計算懲罰因子 P
    long long total_connections = 0;
    for (const auto& conn : data.connections) {
        total_connections += conn.second;
    }
    size_t num_connections = data.connections.size();
    
    // 避免除以零，並設定懲罰因子
    double avg_connection = (num_connections > 0) ? (double)total_connections / num_connections : 1.0;
    double PENALTY_FACTOR = 2.0 * avg_connection;
    double WEIGHT_SF = 2.0; // Soft-Fixed 的額外權重

    cout << "  - Adding Linear Overlap Penalty..." << endl;
    cout << "  - Penalty Factor (P): " << PENALTY_FACTOR << endl;

    GRBLinExpr total_overlap_var = 0;
    int overlap_counter = 0;
    
    // 遍歷所有已定義距離變數的模組對 (Soft-Soft & Soft-Fixed)
    for (const auto& entry : dx_vars) {
        const pair<string, string>& module_pair = entry.first;
        GRBVar dx = entry.second;
        GRBVar dy = dy_vars.at(module_pair);

        string name1 = module_pair.first;
        string name2 = module_pair.second;

        // 查找模組的 s_len
        double s1 = getModuleSize(name1, data);
        double s2 = getModuleSize(name2, data);
        double required_sep = (s1 + s2) / 2.0;
        
        // 引入 Overlap 變數 O_x, O_y (連續變數, >= 0)
        string ox_name = "OX_" + name1 + "_" + name2;
        string oy_name = "OY_" + name1 + "_" + name2;
        GRBVar Ox = model.addVar(0.0, GRB_INFINITY, 0.0, GRB_CONTINUOUS, ox_name);
        GRBVar Oy = model.addVar(0.0, GRB_INFINITY, 0.0, GRB_CONTINUOUS, oy_name);
        
        // 約束條件: Ox >= Required_Sep - Dx
        // 這實現了 Ox = max(0, Required_Sep - Dx)
        model.addConstr(Ox >= required_sep - dx, "OverlapX_" + name1 + "_" + name2);
        model.addConstr(Oy >= required_sep - dy, "OverlapY_" + name1 + "_" + name2);
        
        // 應用權重
        double current_weight = 1.0;
        // 判斷是否為 Soft-Fixed 對 (如果其中一個模組在 fixed_modules 列表中，則為 SF)
        bool is_sf = (data.getModuleByName(name1)->is_soft != data.getModuleByName(name2)->is_soft);
        
        if (is_sf) {
             current_weight = WEIGHT_SF;
        }
        
        total_overlap_var += current_weight * (Ox + Oy);
        overlap_counter++;
    }
    
    // 最終懲罰項 = P * (所有 Overlap 變數的加權總和)
    obj_Penalty = PENALTY_FACTOR * total_overlap_var;
    cout << "  - Total Overlap Pairs modeled: " << overlap_counter << endl;
}


// ----------------------------------------------------------------------
// 儲存結果到 FloorplanData
// ----------------------------------------------------------------------
void GlobalPlacer::storeResults(GRBModel& model) {
    for (auto& m : data.soft_modules) {
        if (x_vars.count(m.name)) {
            // 獲取中心座標
            double x_center = x_vars.at(m.name).get(GRB_DoubleAttr_X);
            double y_center = y_vars.at(m.name).get(GRB_DoubleAttr_X);
            
            // 儲存 Global Placement 結果
            m.global_center.x = (int)round(x_center);
            m.global_center.y = (int)round(y_center);
            
            // 初始化 Local Placement 的 BBox
            m.current_bbox.x_min = (int)round(x_center - m.s_len / 2.0);
            m.current_bbox.y_min = (int)round(y_center - m.s_len / 2.0);
            m.current_bbox.x_max = m.current_bbox.x_min + m.s_len;
            m.current_bbox.y_max = m.current_bbox.y_min + m.s_len;
        }
    }
}