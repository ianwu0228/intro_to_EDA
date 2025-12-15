#include "LocalRefiner.h"
#include <set>      // For std::set
#include <utility>  // For std::pair (儘管通常包含在其他地方，顯式包含更安全)
// ----------------------------------------------------------------------
// 1. 初始化網格 (Initialization)
// ----------------------------------------------------------------------

void LocalRefiner::initializeGrid() {
    // 網格大小為 Chip 寬度 x 高度
    grid.assign(data.chip_h, vector<int>(data.chip_w, 0));
    
    // 標記 Fixed Modules
    for (size_t i = 0; i < data.fixed_modules.size(); ++i) {
        const auto& m = data.fixed_modules[i];
        int module_id = -(i + 1); // 使用負數 ID 標記 Fixed Module
        
        for (int y = m.fixed_y; y < m.fixed_y + m.fixed_h; ++y) {
            for (int x = m.fixed_x; x < m.fixed_x + m.fixed_w; ++x) {
                // 確保在 Chip 範圍內
                if (x >= 0 && x < data.chip_w && y >= 0 && y < data.chip_h) {
                    grid[y][x] = module_id;
                }
            }
        }
    }
    
    // 標記 Soft Modules 的初始 BBox
    for (size_t i = 0; i < data.soft_modules.size(); ++i) {
        auto& m = data.soft_modules[i];
        int module_id = i + 1; // 使用正數 ID 標記 Soft Module

        // 初始面積: 0.64 * min_area, 初始邊長 s_i^(0)
        // 由於 GlobalPlacer 已根據 s_len 初始化 BBox，我們使用 BBox 
        
        for (int y = m.current_bbox.y_min; y < m.current_bbox.y_max; ++y) {
            for (int x = m.current_bbox.x_min; x < m.current_bbox.x_max; ++x) {
                if (x >= 0 && x < data.chip_w && y >= 0 && y < data.chip_h && grid[y][x] == 0) {
                    grid[y][x] = module_id;
                    m.occupied_cells.push_back({x, y});
                }
            }
        }
    }
    cout << "  - Grid initialized. Soft modules start extending." << endl;
}

// ----------------------------------------------------------------------
// 2. 面積檢查 (Area Check)
// ----------------------------------------------------------------------

bool LocalRefiner::isAreaConstraintSatisfied() {
    for (const auto& m : data.soft_modules) {
        if (m.occupied_cells.size() < m.min_area) {
            return false;
        }
    }
    return true;
}

// ----------------------------------------------------------------------
// 3. 延伸方向評分與排序 (Direction Ordering)
// ----------------------------------------------------------------------

vector<Direction> LocalRefiner::getExtensionOrdering(const Module& m) {
    vector<pair<double, Direction>> scores;
    Point center = m.global_center; // 使用 global center 作為錨點判斷方向

    // 評分函數: 使用純 Connection 數 (包含 fixed)
    auto score_direction = [&](Direction dir) -> double {
        double score = 0;
        // 根據方向判斷 BBox 之外的區域與哪些模組中心接近
        
        for (const auto& conn : data.connections) {
            const string& name1 = conn.first.first;
            const string& name2 = conn.first.second;
            int count = conn.second;
            
            const Module* other_m = data.getModuleByName(name1 == m.name ? name2 : name1);
            if (!other_m || other_m->name == m.name) continue;
            
            Point other_center = other_m->is_soft ? other_m->global_center : other_m->global_center;
            
            bool match = false;
            switch (dir) {
                case LEFT:  match = (other_center.x < center.x); break;
                case RIGHT: match = (other_center.x > center.x); break;
                case UP:    match = (other_center.y > center.y); break;
                case DOWN:  match = (other_center.y < center.y); break;
            }
            if (match) {
                score += count; // 使用純 Connection 數評分
            }
        }
        return score;
    };
    
    scores.push_back({score_direction(LEFT), LEFT});
    scores.push_back({score_direction(RIGHT), RIGHT});
    scores.push_back({score_direction(UP), UP});
    scores.push_back({score_direction(DOWN), DOWN});

    // 排序: 依分數降序 (平手隨機)
    sort(scores.begin(), scores.end(), [](const auto& a, const auto& b) {
        if (a.first != b.first) return a.first > b.first;
        // 模擬隨機：分數相同時，不保證順序，讓其自然變化
        return false; 
    });
    
    vector<Direction> ordering;
    for (const auto& p : scores) {
        ordering.push_back(p.second);
    }
    return ordering;
}

// ----------------------------------------------------------------------
// 4. 嘗試延伸 1 格 (Extension Attempt)
// ----------------------------------------------------------------------

bool LocalRefiner::tryExtend(Module& m, Direction dir) {
    int w = data.chip_w;
    int h = data.chip_h;
    
    // 沿 目前 BBox 外邊界 延伸 1 格厚度
    int x_min = m.current_bbox.x_min;
    int y_min = m.current_bbox.y_min;
    int x_max = m.current_bbox.x_max;
    int y_max = m.current_bbox.y_max;
    
    vector<Point> potential_cells;
    
    // 確定要延伸的線段範圍
    if (dir == RIGHT) {
        int x_extend = x_max;
        if (x_extend >= w) return false; // 碰到 chip 邊界
        for (int y = y_min; y < y_max; ++y) { // 延伸整段 BBox 範圍 (y_min..y_max)
            if (grid[y][x_extend] == 0) {
                potential_cells.push_back({x_extend, y});
            }
        }
    } 
    else if (dir == LEFT) {
        int x_extend = x_min - 1;
        if (x_extend < 0) return false; // 碰到 chip 邊界
        for (int y = y_min; y < y_max; ++y) {
            if (grid[y][x_extend] == 0) {
                potential_cells.push_back({x_extend, y});
            }
        }
    }
    else if (dir == UP) {
        int y_extend = y_max;
        if (y_extend >= h) return false; // 碰到 chip 邊界
        for (int x = x_min; x < x_max; ++x) {
            if (grid[y_extend][x] == 0) {
                potential_cells.push_back({x, y_extend});
            }
        }
    }
    else if (dir == DOWN) {
        int y_extend = y_min - 1;
        if (y_extend < 0) return false; // 碰到 chip 邊界
        for (int x = x_min; x < x_max; ++x) {
            if (grid[y_extend][x] == 0) {
                potential_cells.push_back({x, y_extend});
            }
        }
    }
    
    if (potential_cells.empty()) {
        // 該方向整段皆被占據 (0 格可加) -> 該方向不可用
        return false;
    }

    // 執行延伸 (更新網格和 BBox)
    int module_id = distance(data.soft_modules.begin(), find_if(data.soft_modules.begin(), data.soft_modules.end(), 
                                                                [&](const Module& soft_m){ return soft_m.name == m.name; })) + 1;

    for (const auto& p : potential_cells) {
        grid[p.y][p.x] = module_id;
        m.occupied_cells.push_back(p);
    }
    
    // 更新 BBox
    if (dir == RIGHT) m.current_bbox.x_max++;
    else if (dir == LEFT) m.current_bbox.x_min--;
    else if (dir == UP) m.current_bbox.y_max++;
    else if (dir == DOWN) m.current_bbox.y_min--;

    // 更新中心 (for 下一輪的方向評分)
    m.global_center.x = (m.current_bbox.x_min + m.current_bbox.x_max) / 2;
    m.global_center.y = (m.current_bbox.y_min + m.current_bbox.y_max) / 2;
    
    return true;
}

// ----------------------------------------------------------------------
// 5. 執行 Round-Robin 延伸 (Run Extension Rounds)
// ----------------------------------------------------------------------

void LocalRefiner::runExtensionRounds() {
    int round_count = 0;
    
    while (!isAreaConstraintSatisfied()) {
        round_count++;
        int unextended_count = 0;
        
        // 依照 Global Ordering 進行 Round-Robin
        for (auto& m : data.soft_modules) {
            if (m.occupied_cells.size() >= m.min_area) {
                continue; // 已達 minimum area 直接跳過
            }
            
            vector<Direction> ordering = getExtensionOrdering(m);
            bool extended = false;
            
            // 依序嘗試 d1->d2->d3->d4
            for (Direction dir : ordering) {
                if (tryExtend(m, dir)) {
                    extended = true;
                    break; // 成功延伸一次後，換下一個模組
                }
            }
            
            if (!extended) {
                unextended_count++;
            }
        }
        
        cout << "  - Round " << round_count << " finished. Modules not extended: " << unextended_count << endl;
        
        // 停止條件: 所有未達標 modules 回報「無法 extend」
        if (unextended_count == data.soft_modules.size() - count_if(data.soft_modules.begin(), data.soft_modules.end(), 
                                                                    [](const Module& m){ return m.occupied_cells.size() >= m.min_area; })) {
            cout << "  - Local Refinement stopped: Cannot extend remaining modules." << endl;
            break;
        }

        if (round_count > (data.chip_w + data.chip_h) * 2) { 
             cout << "  - Local Refinement stopped: Reached maximum round limit." << endl;
             break;
        }
    }
}

// ----------------------------------------------------------------------
// 6. 最終計算 HPWL (Final HPWL Calculation)
// ----------------------------------------------------------------------

double LocalRefiner::calculateFinalHPWL() const {
    double total_hpwl = 0.0;

    for (const auto& conn : data.connections) {
        const string& name1 = conn.first.first;
        const string& name2 = conn.first.second;
        int count = conn.second;
        
        const Module* m1 = data.getModuleByName(name1);
        const Module* m2 = data.getModuleByName(name2);
        if (!m1 || !m2) continue;
        
        Point c1, c2;

        // 獲取最小包覆矩形的中心 (BBox Center)
        // 對於 Fixed Module，我們在讀取時就計算了 BBox
        
        if (m1->is_soft) {
            c1.x = (m1->current_bbox.x_min + m1->current_bbox.x_max) / 2.0;
            c1.y = (m1->current_bbox.y_min + m1->current_bbox.y_max) / 2.0;
        } else {
            c1 = m1->global_center;
        }

        if (m2->is_soft) {
            c2.x = (m2->current_bbox.x_min + m2->current_bbox.x_max) / 2.0;
            c2.y = (m2->current_bbox.y_min + m2->current_bbox.y_max) / 2.0;
        } else {
            c2 = m2->global_center;
        }

        double dist = manhattanDistance(c1, c2); // 曼哈頓距離
        total_hpwl += dist * count;
    }
    return total_hpwl;
}

// ----------------------------------------------------------------------
// 7. 轉角生成 (Corner Generation)
// ----------------------------------------------------------------------

const int dx[] = {1, 0, -1, 0};
const int dy[] = {0, 1, 0, -1};

// ----------------------------------------------------------------------
// 7. 轉角生成 (Corner Generation) - 使用邊界追蹤 (Boundary Tracing)
// ----------------------------------------------------------------------

vector<Point> LocalRefiner::generateCorners(const Module& m) const {
    vector<Point> corners;
    
    // 獲取模組 ID
    // 由於 m 是 const 引用，我們需要手動查找 ID
    int module_id = 0;
    for (size_t i = 0; i < data.soft_modules.size(); ++i) {
        if (data.soft_modules[i].name == m.name) {
            module_id = i + 1;
            break;
        }
    }
    if (module_id == 0) return corners; // 找不到模組

    // 將 occupied_cells 轉換為快速查找的 set
    set<pair<int, int>> occupied_set;
    for (const auto& p : m.occupied_cells) {
        occupied_set.insert({p.x, p.y});
    }

    // 輔助函數: 檢查 (x, y) 格子是否被當前模組佔據
    auto is_occupied = [&](int x, int y) -> bool {
        // 檢查座標範圍
        if (x < 0 || x >= data.chip_w || y < 0 || y >= data.chip_h) return false;
        // 檢查 grid ID
        return grid[y][x] == module_id;
    };

    // 1. 找到起始點 (最左下角的格子的左下角座標)
    // 找到所有佔據格子中最左邊的 x_min 和最下方的 y_min
    int start_gx = m.current_bbox.x_min; // Grid X
    int start_gy = m.current_bbox.y_min; // Grid Y
    
    // 必須從 BBox 的左下角開始，並找到第一個被佔據的格子 (用於確定邊界)
    while (!is_occupied(start_gx, start_gy) && start_gx < m.current_bbox.x_max) start_gx++;
    while (!is_occupied(start_gx, start_gy) && start_gy < m.current_bbox.y_max) start_gy++;

    // 轉角追蹤從網格線的交點開始 (即格子座標)
    int current_x = start_gx;
    int current_y = start_gy;
    
    // 初始方向：向上 (1)，目標是從左下角開始追蹤
    // 我們從 (start_x, start_y) 點的左下方開始追蹤其左邊界
    int current_dir = UP; 
    
    // 為了確保我們從一個確定的外部邊界開始，我們從 (start_gx, start_gy) 的左側邊界開始。
    // 起始位置 (Start Point)
    current_x = start_gx;
    current_y = start_gy; 

    // 追蹤時，初始位置是第一個轉角 (Start Corner)
    int start_corner_x = current_x;
    int start_corner_y = current_y;
    int initial_dir = current_dir; 
    bool first_corner_found = false;

    // 迴圈開始 (最多 W*H*4 步)
    int safety_counter = (data.chip_w + data.chip_h) * 10; 
    
    do {
        // 紀錄當前點 (可能是轉角)
        if (first_corner_found) {
             if (corners.empty() || corners.back().x != current_x || corners.back().y != current_y) {
                corners.push_back({current_x, current_y});
            }
        }
        
        // 決定下一個方向 (左手定則 - 順時針追蹤外邊界)
        // 優先嘗試右轉 (相對於當前方向 - 1)，然後直行，最後左轉 (相對於當前方向 + 1)
        
        int next_dir = current_dir;
        bool found_next_step = false;

        // 嘗試 4 個方向 (從當前方向的左側開始逆時針嘗試: 3, 0, 1, 2)
        // 3: 左轉 (優先轉角), 0: 直行, 1: 右轉, 2: 倒退
        for (int j = 0; j < 4; ++j) {
            // 嘗試方向: (current_dir - 1 + j + 4) % 4
            // 這是逆時針的嘗試順序 (Left, Straight, Right, Back)
            int try_dir = (current_dir + 3 + j) % 4; // 3: 左轉, 0: 直行, 1: 右轉, 2: 倒退
            
            // 檢查 P_front: 沿 try_dir 移動一步 (線段終點)
            int Pfront_x = current_x + dx[try_dir];
            int Pfront_y = current_y + dy[try_dir];

            // 檢查 P_left (內側): P_front 沿 (try_dir + 1) 移動一步
            int Pleft_dir = (try_dir + 1) % 4; 
            int Pleft_x = Pfront_x + dx[Pleft_dir];
            int Pleft_y = Pfront_y + dy[Pleft_dir];
            
            // 追蹤規則 (左手定則): 順時針追蹤外部邊界時，模組在左邊
            // P_left 必須是模組內部 (is_occupied)
            // P_front (線段本身) 必須與 P_right (外部) 相鄰

            // 修正：在邊界追蹤中，我們檢查的是**兩個相鄰格子**的佔據狀態。
            // 檢查 P_in (模組內側) 和 P_out (模組外側)
            int Pin_x, Pin_y; // 內側格子 (模組)
            int Pout_x, Pout_y; // 外側格子 (空地)

            // 順時針追蹤時，P_out 在線段的右側
            Pin_x = current_x + dx[try_dir]; Pin_y = current_y + dy[try_dir]; // (簡化)
            Pout_x = Pin_x + dx[(try_dir + 3) % 4]; Pout_y = Pin_y + dy[(try_dir + 3) % 4];

            // 追蹤條件：
            // 1. 沿 try_dir 移動一步後的 P_in 必須是模組 AND
            // 2. P_out 必須是空地
            // 這是找到邊界線的基本條件

            // 實際追蹤中，我們使用兩個點來定義當前邊界：
            // P_current: 模組內部點 (位於當前追蹤線段的左上/右上等)
            // P_next: 模組內部點 (沿追蹤方向)

            // 為了簡化，我們使用一個更穩定的檢查：
            // **向右轉** (左手定則): P_right 必須是空地
            // **直行/左轉**: 檢查前方格子是否可用

            // 檢查模組內側 (P_in) 和模組外側 (P_out)
            int P_in_x = current_x + dx[try_dir];
            int P_in_y = current_y + dy[try_dir];
            
            // 外側方向是 (try_dir - 1 + 4) % 4
            int P_out_dir = (try_dir + 3) % 4;
            int P_out_x = current_x + dx[P_out_dir];
            int P_out_y = current_y + dy[P_out_dir];
            
            // 追蹤邏輯：
            // 1. 如果 P_out 是空地且 P_in 是模組，則嘗試轉向 (Left Hand Rule)
            // 2. 如果 P_out 是模組且 P_in 是空地，則不能移動

            // 順時針 (CW): 模組在左邊。
            // 如果 P_in (線段左側) 是模組 AND P_out (線段右側) 是空地
            if (is_occupied(P_in_x, P_in_y) && !is_occupied(P_out_x, P_out_y)) {
                // 找到下一步！
                next_dir = try_dir;
                found_next_step = true;
                break;
            }
        }
        
        if (!found_next_step) {
            // 找不到下一步，可能是孤立的模組或錯誤
            break; 
        }

        // 紀錄轉角 (方向改變時)
        if (current_dir != next_dir) {
            if (first_corner_found) {
                // 如果是轉向，紀錄轉角
                corners.push_back({current_x, current_y});
            }
            current_dir = next_dir;
            first_corner_found = true;
        }

        // 移動到下一個點
        current_x += dx[current_dir];
        current_y += dy[current_dir];

        safety_counter--;

    } while ((current_x != start_corner_x || current_y != start_corner_y) && safety_counter > 0);

    // 迴圈結束時，紀錄起始點作為最後一個轉角
    if (first_corner_found && !corners.empty()) {
        corners.push_back({start_corner_x, start_corner_y});
    }

    // 移除重複點
    vector<Point> unique_corners;
    if (!corners.empty()) {
        unique_corners.push_back(corners[0]);
        for (size_t i = 1; i < corners.size(); ++i) {
            if (corners[i].x != corners[i-1].x || corners[i].y != corners[i-1].y) {
                unique_corners.push_back(corners[i]);
            }
        }
        // 處理首尾點重複（如果迴圈正確結束，理論上應該只有一個首尾點）
        if (unique_corners.size() > 1 && unique_corners.front().x == unique_corners.back().x && 
            unique_corners.front().y == unique_corners.back().y) {
            unique_corners.pop_back();
        }
    }

    // 最終檢查：確保至少有 4 個點 (如果模組是矩形)
    if (unique_corners.size() < 3) {
        // 如果追蹤失敗，使用 BBox 作為回退 (fallback) 輸出 (仍是順時針)
        unique_corners.clear();
        unique_corners.push_back({m.current_bbox.x_min, m.current_bbox.y_min}); // 1. 左下
        unique_corners.push_back({m.current_bbox.x_max, m.current_bbox.y_min}); // 2. 右下
        unique_corners.push_back({m.current_bbox.x_max, m.current_bbox.y_max}); // 3. 右上
        unique_corners.push_back({m.current_bbox.x_min, m.current_bbox.y_max}); // 4. 左上
    }

    return unique_corners;
}


// ----------------------------------------------------------------------
// 8. 寫入輸出檔案 (Write Output)
// ----------------------------------------------------------------------

void LocalRefiner::writeOutput(const string& output_filename, double hpwl) {
    ofstream ofs(output_filename);
    ofs.precision(1);
    ofs << fixed;

    // HPWL
    ofs << "HPWL " << hpwl << endl;

    // SOFTMODULE 數量
    ofs << "SOFTMODULE " << data.soft_modules.size() << endl;

    for (const auto& m : data.soft_modules) {
        vector<Point> corners = generateCorners(m); // 生成轉角
        
        // 模組名稱和轉角數量
        ofs << m.name << " " << corners.size() << endl;

        // 轉角座標 (順時針)
        for (const auto& p : corners) {
            ofs << p.x << " " << p.y << endl;
        }
    }

    ofs.close();
}


// ----------------------------------------------------------------------
// 公共介面 (Public Interface)
// ----------------------------------------------------------------------

void LocalRefiner::refineAndOutput(const string& output_filename) {
    cout << "\n--- Starting Local Refinement ---" << endl;
    
    // 1. 初始化網格和模組
    initializeGrid();
    
    // 2. 執行 Round-Robin 延伸
    runExtensionRounds();
    
    // 3. 最終計算 HPWL
    double final_hpwl = calculateFinalHPWL();
    cout << "Final HPWL: " << final_hpwl << endl;
    
    // 4. 寫入輸出檔案
    writeOutput(output_filename, final_hpwl);
    
    cout << "--- Local Refinement Finished ---\n" << endl;
}