// refiner.cpp
// Stage-2 refinement (independent of stage-1 algorithm):
//   - Input 1: original problem input (CHIP / SOFTMODULE / FIXEDMODULE / CONNECTION)
//   - Input 2: stage-1 output (HPWL ... SOFTMODULE ... polygons for soft modules)
//   - Output : refined placement (rectangles) with reduced HPWL (greedy expansion)
//
// This version avoids:
//   - aggregate brace-init (Rect{...}, Connection{...})
//   - structured bindings (auto [a,b])
// so it compiles cleanly even if your toolchain is picky.
//
// Build (standalone):
//   g++ -O2 -std=c++17 refiner.cpp -DREFINE_STANDALONE -o refiner
// Run:
//   ./refiner <input.txt> <stage1.out> <result.out>
//
// Example:
//   ./refiner ../input/case01-input.txt ../output/ver_08/case01.out ./result.out

#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstdint>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <numeric>
#include <sstream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace refine2
{

    static const int16_t EMPTY = -1;
    static const int16_t FIXED = -2;

    struct Rect
    {
        int x1, y1, x2, y2; // half-open: [x1,x2) x [y1,y2)
        Rect() : x1(0), y1(0), x2(0), y2(0) {}
    };

    static inline int rect_w(const Rect &r) { return r.x2 - r.x1; }
    static inline int rect_h(const Rect &r) { return r.y2 - r.y1; }
    static inline double rect_cx(const Rect &r) { return 0.5 * (double(r.x1) + double(r.x2)); }
    static inline double rect_cy(const Rect &r) { return 0.5 * (double(r.y1) + double(r.y2)); }

    static inline bool aspect_ok(const Rect &r)
    {
        const double w = double(rect_w(r));
        const double h = double(rect_h(r));
        if (w <= 0.0 || h <= 0.0)
            return false;
        const double ar = h / w; // h/w in [0.5, 2]
        return (ar >= 0.5 && ar <= 2.0);
    }

    struct SoftSpec
    {
        std::string name;
        int minArea;
        SoftSpec() : name(), minArea(0) {}
    };

    struct FixedMod
    {
        std::string name;
        Rect r;
    };

    struct Connection
    {
        int a, b;
        int w;
        Connection() : a(-1), b(-1), w(0) {}
    };

    struct Problem
    {
        int W, H;
        std::vector<SoftSpec> soft;
        std::vector<FixedMod> fixed;
        std::vector<Connection> conns;
        std::vector<std::vector<std::pair<int, int>>> adj; // adj[i] = (neighbor, weight)
        Problem() : W(0), H(0) {}
    };

    struct Stage1Placement
    {
        std::vector<Rect> softRects; // MBR-based rectangles for each soft module
    };

    static std::vector<std::string> read_tokens(const std::string &path)
    {
        std::ifstream ifs(path.c_str());
        if (!ifs)
            throw std::runtime_error("Cannot open file: " + path);
        std::vector<std::string> toks;
        std::string s;
        while (ifs >> s)
            toks.push_back(s);
        return toks;
    }

    static Problem parse_input_problem(const std::string &inputPath)
    {
        Problem pb;
        std::vector<std::string> t = read_tokens(inputPath);
        size_t i = 0;

        auto need = [&](bool cond, const std::string &msg)
        {
            if (!cond)
                throw std::runtime_error("Parse error (input): " + msg);
        };

        // 1st pass: CHIP / SOFTMODULE / FIXEDMODULE
        while (i < t.size())
        {
            const std::string tok = t[i++];

            if (tok == "CHIP")
            {
                need(i + 1 < t.size(), "CHIP needs W H");
                pb.W = std::stoi(t[i++]);
                pb.H = std::stoi(t[i++]);
                need(pb.W > 0 && pb.H > 0, "Invalid CHIP size");
            }
            else if (tok == "SOFTMODULE")
            {
                need(i < t.size(), "SOFTMODULE needs count");
                const int n = std::stoi(t[i++]);
                need(n >= 0, "Invalid SOFTMODULE count");
                pb.soft.resize(n);
                for (int k = 0; k < n; ++k)
                {
                    need(i + 1 < t.size(), "SOFTMODULE entry needs name minArea");
                    pb.soft[k].name = t[i++];
                    pb.soft[k].minArea = std::stoi(t[i++]);
                    need(pb.soft[k].minArea > 0, "minArea must be > 0");
                }
            }
            else if (tok == "FIXEDMODULE")
            {
                need(i < t.size(), "FIXEDMODULE needs count");
                const int m = std::stoi(t[i++]);
                need(m >= 0, "Invalid FIXEDMODULE count");
                pb.fixed.reserve(m);
                for (int k = 0; k < m; ++k)
                {
                    need(i + 4 < t.size(), "FIXEDMODULE entry needs name x y w h");
                    FixedMod fm;
                    fm.name = t[i++];
                    const int x = std::stoi(t[i++]);
                    const int y = std::stoi(t[i++]);
                    const int w = std::stoi(t[i++]);
                    const int h = std::stoi(t[i++]);
                    need(x >= 0 && y >= 0 && w > 0 && h > 0, "Invalid FIXEDMODULE geometry");
                    fm.r.x1 = x;
                    fm.r.y1 = y;
                    fm.r.x2 = x + w;
                    fm.r.y2 = y + h;
                    pb.fixed.push_back(fm);
                }
            }
            else
            {
                // ignore others in first pass (we parse CONNECTION in second pass)
            }
        }

        // Build name->id map for soft modules
        std::unordered_map<std::string, int> softId;
        softId.reserve(pb.soft.size() * 2 + 8);
        for (int id = 0; id < (int)pb.soft.size(); ++id)
            softId[pb.soft[id].name] = id;

        // 2nd pass: parse CONNECTION
        t = read_tokens(inputPath);
        i = 0;
        pb.conns.clear();

        while (i < t.size())
        {
            const std::string tok = t[i++];

            if (tok == "CONNECTION")
            {
                need(i < t.size(), "CONNECTION needs count");
                const int c = std::stoi(t[i++]);
                need(c >= 0, "Invalid CONNECTION count");
                pb.conns.reserve(c);

                for (int k = 0; k < c; ++k)
                {
                    need(i + 2 < t.size(), "CONNECTION entry needs A B W");
                    const std::string aName = t[i++];
                    const std::string bName = t[i++];
                    const int w = std::stoi(t[i++]);
                    if (w <= 0)
                        continue;

                    std::unordered_map<std::string, int>::const_iterator ia = softId.find(aName);
                    std::unordered_map<std::string, int>::const_iterator ib = softId.find(bName);
                    need(ia != softId.end() && ib != softId.end(),
                         "CONNECTION references unknown module: " + aName + " or " + bName);

                    Connection e;
                    e.a = ia->second;
                    e.b = ib->second;
                    e.w = w;
                    pb.conns.push_back(e);
                }
            }
        }

        // Build adjacency
        pb.adj.assign(pb.soft.size(), std::vector<std::pair<int, int>>());
        for (size_t k = 0; k < pb.conns.size(); ++k)
        {
            const Connection &e = pb.conns[k];
            pb.adj[e.a].push_back(std::make_pair(e.b, e.w));
            pb.adj[e.b].push_back(std::make_pair(e.a, e.w));
        }

        return pb;
    }

    static Stage1Placement parse_stage1_output(const std::string &stage1Path, const Problem &pb)
    {
        std::vector<std::string> t = read_tokens(stage1Path);
        size_t i = 0;

        auto need = [&](bool cond, const std::string &msg)
        {
            if (!cond)
                throw std::runtime_error("Parse error (stage1 output): " + msg);
        };

        std::unordered_map<std::string, int> softId;
        softId.reserve(pb.soft.size() * 2 + 8);
        for (int id = 0; id < (int)pb.soft.size(); ++id)
            softId[pb.soft[id].name] = id;

        // Optional "HPWL <value>"
        if (i < t.size() && t[i] == "HPWL")
        {
            i++;
            need(i < t.size(), "HPWL needs value");
            i++; // skip value
        }

        need(i < t.size() && t[i] == "SOFTMODULE", "Missing SOFTMODULE section");
        i++;
        need(i < t.size(), "SOFTMODULE needs count");
        const int n = std::stoi(t[i++]);
        need(n == (int)pb.soft.size(), "Stage1 SOFTMODULE count mismatch");

        Stage1Placement st;
        st.softRects.assign(pb.soft.size(), Rect());

        for (int k = 0; k < n; ++k)
        {
            need(i + 1 < t.size(), "Soft entry needs name numCorners");
            const std::string name = t[i++];
            const int corners = std::stoi(t[i++]);
            need(corners >= 4, "numCorners must be >= 4");

            std::unordered_map<std::string, int>::const_iterator it = softId.find(name);
            need(it != softId.end(), "Unknown soft name in stage1 output: " + name);
            const int id = it->second;

            int minx = std::numeric_limits<int>::max();
            int miny = std::numeric_limits<int>::max();
            int maxx = std::numeric_limits<int>::min();
            int maxy = std::numeric_limits<int>::min();

            need(i + (size_t)corners * 2 <= t.size(), "Not enough point tokens");
            for (int c = 0; c < corners; ++c)
            {
                const int x = std::stoi(t[i++]);
                const int y = std::stoi(t[i++]);
                minx = std::min(minx, x);
                miny = std::min(miny, y);
                maxx = std::max(maxx, x);
                maxy = std::max(maxy, y);
            }

            Rect r;
            r.x1 = minx;
            r.y1 = miny;
            r.x2 = maxx;
            r.y2 = maxy;

            need(r.x1 >= 0 && r.y1 >= 0, "Negative coordinate in stage1 output");
            need(r.x2 > r.x1 && r.y2 > r.y1, "Degenerate rectangle from polygon MBR");

            st.softRects[id] = r;
        }

        return st;
    }

    // -------- Grid --------

    struct Grid
    {
        int W, H;
        std::vector<int16_t> occ;

        Grid(int w, int h) : W(w), H(h), occ((size_t)w * (size_t)h, EMPTY) {}

        inline int idx(int x, int y) const { return y * W + x; }
        inline int16_t get(int x, int y) const { return occ[(size_t)idx(x, y)]; }
        inline void set(int x, int y, int16_t v) { occ[(size_t)idx(x, y)] = v; }
    };

    static void validate_initial(const Problem &pb, const std::vector<Rect> &softRects)
    {
        for (size_t i = 0; i < softRects.size(); ++i)
        {
            const Rect &r = softRects[i];
            if (!(0 <= r.x1 && r.x1 < r.x2 && r.x2 <= pb.W &&
                  0 <= r.y1 && r.y1 < r.y2 && r.y2 <= pb.H))
            {
                throw std::runtime_error("Initial soft rect out of chip: " + pb.soft[i].name);
            }
            if (!aspect_ok(r))
            {
                throw std::runtime_error("Initial soft rect violates aspect ratio [0.5,2]: " + pb.soft[i].name);
            }
            const long long area = 1LL * rect_w(r) * rect_h(r);
            if (area < pb.soft[i].minArea)
            {
                throw std::runtime_error("Initial soft rect area < minArea: " + pb.soft[i].name);
            }
        }
    }

    static double total_hpwl(const Problem &pb, const std::vector<Rect> &rects)
    {
        double s = 0.0;
        for (size_t k = 0; k < pb.conns.size(); ++k)
        {
            const Connection &e = pb.conns[k];
            const double dx = std::fabs(rect_cx(rects[e.a]) - rect_cx(rects[e.b]));
            const double dy = std::fabs(rect_cy(rects[e.a]) - rect_cy(rects[e.b]));
            s += (dx + dy) * double(e.w);
        }
        return s;
    }

    static double delta_hpwl_for_move(const Problem &pb, int i, const Rect &oldR, const Rect &newR,
                                      const std::vector<Rect> &rects)
    {
        double d = 0.0;
        const double ox = rect_cx(oldR), oy = rect_cy(oldR);
        const double nx = rect_cx(newR), ny = rect_cy(newR);

        const std::vector<std::pair<int, int>> &v = pb.adj[i];
        for (size_t k = 0; k < v.size(); ++k)
        {
            const int j = v[k].first;
            const int w = v[k].second;

            const double jx = rect_cx(rects[j]);
            const double jy = rect_cy(rects[j]);

            const double oldD = std::fabs(ox - jx) + std::fabs(oy - jy);
            const double newD = std::fabs(nx - jx) + std::fabs(ny - jy);
            d += (newD - oldD) * double(w);
        }
        return d;
    }

    enum Dir
    {
        LEFT = 0,
        RIGHT = 1,
        DOWN = 2,
        UP = 3
    };

    static Dir primary_dir_for_module(const Problem &pb, int i, const std::vector<Rect> &rects)
    {
        double sumW = 0.0, tx = 0.0, ty = 0.0;
        const std::vector<std::pair<int, int>> &v = pb.adj[i];
        for (size_t k = 0; k < v.size(); ++k)
        {
            const int j = v[k].first;
            const int w = v[k].second;
            sumW += double(w);
            tx += double(w) * rect_cx(rects[j]);
            ty += double(w) * rect_cy(rects[j]);
        }
        if (sumW <= 0.0)
            return RIGHT;

        tx /= sumW;
        ty /= sumW;
        const double vx = tx - rect_cx(rects[i]);
        const double vy = ty - rect_cy(rects[i]);

        if (std::fabs(vx) >= std::fabs(vy))
            return (vx >= 0.0) ? RIGHT : LEFT;
        return (vy >= 0.0) ? UP : DOWN;
    }

    static double priority_score(const Problem &pb, int i, const std::vector<Rect> &rects)
    {
        double sumW = 0.0, tx = 0.0, ty = 0.0;
        const std::vector<std::pair<int, int>> &v = pb.adj[i];
        for (size_t k = 0; k < v.size(); ++k)
        {
            const int j = v[k].first;
            const int w = v[k].second;
            sumW += double(w);
            tx += double(w) * rect_cx(rects[j]);
            ty += double(w) * rect_cy(rects[j]);
        }
        if (sumW <= 0.0)
            return 0.0;
        tx /= sumW;
        ty /= sumW;
        const double vx = tx - rect_cx(rects[i]);
        const double vy = ty - rect_cy(rects[i]);
        return sumW * (std::fabs(vx) + std::fabs(vy));
    }

    static void dir_order(Dir primary, Dir out[4])
    {
        // primary, two perpendiculars, opposite
        if (primary == LEFT)
        {
            out[0] = LEFT;
            out[1] = UP;
            out[2] = DOWN;
            out[3] = RIGHT;
            return;
        }
        if (primary == RIGHT)
        {
            out[0] = RIGHT;
            out[1] = UP;
            out[2] = DOWN;
            out[3] = LEFT;
            return;
        }
        if (primary == UP)
        {
            out[0] = UP;
            out[1] = LEFT;
            out[2] = RIGHT;
            out[3] = DOWN;
            return;
        }
        // DOWN
        out[0] = DOWN;
        out[1] = LEFT;
        out[2] = RIGHT;
        out[3] = UP;
    }

    static Rect expanded_rect(const Rect &r, Dir d)
    {
        Rect nr = r;
        if (d == RIGHT)
            nr.x2 += 1;
        else if (d == LEFT)
            nr.x1 -= 1;
        else if (d == UP)
            nr.y2 += 1;
        else
            nr.y1 -= 1; // DOWN
        return nr;
    }

    static bool strip_empty(const Grid &g, const Rect &oldR, Dir d)
    {
        if (d == RIGHT)
        {
            const int x = oldR.x2;
            if (x < 0 || x >= g.W)
                return false;
            for (int y = oldR.y1; y < oldR.y2; ++y)
                if (g.get(x, y) != EMPTY)
                    return false;
            return true;
        }
        if (d == LEFT)
        {
            const int x = oldR.x1 - 1;
            if (x < 0 || x >= g.W)
                return false;
            for (int y = oldR.y1; y < oldR.y2; ++y)
                if (g.get(x, y) != EMPTY)
                    return false;
            return true;
        }
        if (d == UP)
        {
            const int y = oldR.y2;
            if (y < 0 || y >= g.H)
                return false;
            for (int x = oldR.x1; x < oldR.x2; ++x)
                if (g.get(x, y) != EMPTY)
                    return false;
            return true;
        }
        // DOWN
        const int y = oldR.y1 - 1;
        if (y < 0 || y >= g.H)
            return false;
        for (int x = oldR.x1; x < oldR.x2; ++x)
            if (g.get(x, y) != EMPTY)
                return false;
        return true;
    }

    static void paint_new_strip(Grid &g, const Rect &oldR, Dir d, int softId)
    {
        const int16_t id = (int16_t)softId;
        if (d == RIGHT)
        {
            const int x = oldR.x2;
            for (int y = oldR.y1; y < oldR.y2; ++y)
                g.set(x, y, id);
            return;
        }
        if (d == LEFT)
        {
            const int x = oldR.x1 - 1;
            for (int y = oldR.y1; y < oldR.y2; ++y)
                g.set(x, y, id);
            return;
        }
        if (d == UP)
        {
            const int y = oldR.y2;
            for (int x = oldR.x1; x < oldR.x2; ++x)
                g.set(x, y, id);
            return;
        }
        // DOWN
        const int y = oldR.y1 - 1;
        for (int x = oldR.x1; x < oldR.x2; ++x)
            g.set(x, y, id);
    }

    static void build_grid_or_throw(const Problem &pb, const std::vector<Rect> &softRects, Grid &grid)
    {
        // Paint fixed
        for (size_t i = 0; i < pb.fixed.size(); ++i)
        {
            const FixedMod &fm = pb.fixed[i];
            const Rect &r = fm.r;
            if (!(0 <= r.x1 && r.x1 < r.x2 && r.x2 <= pb.W &&
                  0 <= r.y1 && r.y1 < r.y2 && r.y2 <= pb.H))
            {
                throw std::runtime_error("Fixed module out of chip: " + fm.name);
            }
            for (int y = r.y1; y < r.y2; ++y)
            {
                for (int x = r.x1; x < r.x2; ++x)
                {
                    if (grid.get(x, y) != EMPTY)
                        throw std::runtime_error("Fixed overlaps fixed: " + fm.name);
                    grid.set(x, y, FIXED);
                }
            }
        }

        // Paint soft
        for (int sid = 0; sid < (int)softRects.size(); ++sid)
        {
            const Rect &r = softRects[sid];
            for (int y = r.y1; y < r.y2; ++y)
            {
                for (int x = r.x1; x < r.x2; ++x)
                {
                    if (grid.get(x, y) != EMPTY)
                        throw std::runtime_error("Initial soft overlaps: " + pb.soft[sid].name);
                    grid.set(x, y, (int16_t)sid);
                }
            }
        }
    }

    static void refine_grow_rectangles(const Problem &pb,
                                       std::vector<Rect> &softRects,
                                       Grid &grid,
                                       int passes,
                                       int maxMovesPerModulePerPass)
    {
        const int n = (int)softRects.size();

        for (int pass = 0; pass < passes; ++pass)
        {
            std::vector<int> order(n);
            std::iota(order.begin(), order.end(), 0);

            std::stable_sort(order.begin(), order.end(),
                             [&](int a, int b)
                             { return priority_score(pb, a, softRects) > priority_score(pb, b, softRects); });

            bool any = false;

            for (int oi = 0; oi < n; ++oi)
            {
                const int i = order[oi];
                if (pb.adj[i].empty())
                    continue;

                int moves = 0;
                while (moves < maxMovesPerModulePerPass)
                {
                    const Dir primary = primary_dir_for_module(pb, i, softRects);
                    Dir dirs[4];
                    dir_order(primary, dirs);

                    bool moved = false;
                    for (int k = 0; k < 4; ++k)
                    {
                        const Dir d = dirs[k];
                        const Rect oldR = softRects[i];
                        const Rect newR = expanded_rect(oldR, d);

                        // boundary
                        if (!(0 <= newR.x1 && newR.x1 < newR.x2 && newR.x2 <= pb.W &&
                              0 <= newR.y1 && newR.y1 < newR.y2 && newR.y2 <= pb.H))
                        {
                            continue;
                        }

                        // aspect ratio
                        if (!aspect_ok(newR))
                            continue;

                        // new strip empty?
                        if (!strip_empty(grid, oldR, d))
                            continue;

                        // delta HPWL
                        const double dHPWL = delta_hpwl_for_move(pb, i, oldR, newR, softRects);
                        if (!(dHPWL < -1e-9))
                            continue;

                        // accept
                        paint_new_strip(grid, oldR, d, i);
                        softRects[i] = newR;
                        any = true;
                        moved = true;
                        moves++;
                        break;
                    }

                    if (!moved)
                        break;
                }
            }

            if (!any)
                break; // converged
        }
    }

    static void write_output(const std::string &outPath, const Problem &pb, const std::vector<Rect> &softRects)
    {
        std::ofstream ofs(outPath.c_str());
        if (!ofs)
            throw std::runtime_error("Cannot open output: " + outPath);

        const double hpwl = total_hpwl(pb, softRects);

        ofs << "HPWL " << std::fixed << std::setprecision(1) << hpwl << "\n";
        ofs << "SOFTMODULE " << pb.soft.size() << "\n";

        for (size_t i = 0; i < pb.soft.size(); ++i)
        {
            const Rect &r = softRects[i];
            ofs << pb.soft[i].name << " 4\n";
            ofs << r.x1 << " " << r.y1 << "\n";
            ofs << r.x2 << " " << r.y1 << "\n";
            ofs << r.x2 << " " << r.y2 << "\n";
            ofs << r.x1 << " " << r.y2 << "\n";
        }
    }

    int run(const std::string &inputPath,
            const std::string &stage1OutPath,
            const std::string &outPath,
            int passes,
            int maxMovesPerModulePerPass)
    {
        Problem pb = parse_input_problem(inputPath);
        Stage1Placement st = parse_stage1_output(stage1OutPath, pb);

        std::vector<Rect> softRects = st.softRects;
        validate_initial(pb, softRects);

        Grid grid(pb.W, pb.H);
        build_grid_or_throw(pb, softRects, grid);

        // Tune these if you want “stronger” refinement
        refine_grow_rectangles(pb, softRects, grid, passes, maxMovesPerModulePerPass);

        write_output(outPath, pb, softRects);
        return 0;
    }

} // namespace refine2

#ifdef REFINE_STANDALONE
int main(int argc, char **argv)
{
    if (argc < 4)
    {
        std::cerr << "Usage: " << argv[0] << " <input.txt> <stage1.out> <result.out>\n";
        return 1;
    }
    try
    {
        // Default tuning
        const int passes = 6;
        const int maxMovesPerModulePerPass = 2000;
        return refine2::run(argv[1], argv[2], argv[3], passes, maxMovesPerModulePerPass);
    }
    catch (const std::exception &e)
    {
        std::cerr << "ERROR: " << e.what() << "\n";
        return 2;
    }
}
#endif
