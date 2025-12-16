#include <bits/stdc++.h>
using namespace std;

/*
  Build:
    g++ -O3 -march=native -std=c++17 refiner_pixel.cpp -o refiner_pixel

  Run:
    ./refiner_pixel <input_problem.txt> <stage1.out> <final.out>

  Input problem format (your caseXX-input.txt):
    CHIP W H
    SOFTMODULE N
      <name> <minArea>
    FIXEDMODULE M
      <name> x y w h
    CONNECTION E
      <name1> <name2> <weight>

  Stage1 output format (your caseXX.out):
    HPWL <value>
    SOFTMODULE <N>
    <name> <k>
    x y
    ... (k lines)
*/

enum class ModType
{
    SOFT,
    FIXED
};

struct Module
{
    int id = -1;
    string name;
    ModType type = ModType::SOFT;

    long long minArea = 0;

    // Occupied area (in unit cells)
    long long area = 0;

    // Bounding box in half-open coords: [minx,maxx) x [miny,maxy)
    int minx = 0, miny = 0, maxx = 0, maxy = 0;

    // Priority/force direction
    double fx = 0.0, fy = 0.0;

    // Frontier as a bag of packed cells; we validate via inFrontier bitset
    vector<int> frontier;
};

struct Edge
{
    int to;
    int w;
};
struct Net2
{
    int a, b, w;
};

static inline int packCell(int x, int y, int W) { return y * W + x; }
static inline int cellX(int p, int W) { return p % W; }
static inline int cellY(int p, int W) { return p / W; }

static inline long long keyVertex(int x, int y)
{
    return ((long long)x << 32) ^ (unsigned int)y;
}

class RefinerPixel
{
public:
    int chipW = 0, chipH = 0;
    vector<Module> mods;
    unordered_map<string, int> name2id;

    vector<Net2> nets;
    vector<vector<Edge>> adj;

    // grid cell owner: -1 empty else module id
    vector<int> grid;

    // inFrontier[m][cell] => bool (stored as vector<uint8_t>)
    vector<vector<uint8_t>> inFrontier;

    // Tunables
    int maxRounds = 1000;
    int maxStepsPerModPerRound = 200;

    // constraints
    double aspectMin = 0.5;
    double aspectMax = 2.0;
    double rectRatioMin = 0.80;

    // acceptance
    double hpwlEps = 0.0;     // allow small worsening; try 0, 100, 1000
    bool allowNeutral = true; // allow dHPWL == 0

    // direction bias
    double dirBias = 1e-3; // tiny

    // -------- parsing --------
    void parseProblem(const string &filename)
    {
        ifstream in(filename);
        if (!in)
            throw runtime_error("Cannot open input problem: " + filename);

        string tok;
        while (in >> tok)
        {
            if (tok == "CHIP")
            {
                in >> chipW >> chipH;
            }
            else if (tok == "SOFTMODULE")
            {
                int n;
                in >> n;
                for (int i = 0; i < n; i++)
                {
                    string nm;
                    long long minA;
                    in >> nm >> minA;
                    Module m;
                    m.id = (int)mods.size();
                    m.name = nm;
                    m.type = ModType::SOFT;
                    m.minArea = minA;
                    mods.push_back(std::move(m));
                    name2id[nm] = mods.back().id;
                }
            }
            else if (tok == "FIXEDMODULE")
            {
                int m;
                in >> m;
                for (int i = 0; i < m; i++)
                {
                    string nm;
                    int x, y, w, h;
                    in >> nm >> x >> y >> w >> h;
                    Module md;
                    md.id = (int)mods.size();
                    md.name = nm;
                    md.type = ModType::FIXED;
                    md.minArea = 1LL * w * h;
                    md.area = md.minArea;
                    md.minx = x;
                    md.miny = y;
                    md.maxx = x + w;
                    md.maxy = y + h;
                    mods.push_back(std::move(md));
                    name2id[nm] = mods.back().id;
                }
            }
            else if (tok == "CONNECTION")
            {
                int e;
                in >> e;
                nets.reserve(e);
                for (int i = 0; i < e; i++)
                {
                    string a, b;
                    int w;
                    in >> a >> b >> w;
                    int ia = name2id.at(a);
                    int ib = name2id.at(b);
                    nets.push_back({ia, ib, w});
                }
            }
            else
            {
                // ignore unknown tokens
            }
        }

        // adjacency
        adj.assign(mods.size(), {});
        for (auto &n : nets)
        {
            adj[n.a].push_back({n.b, n.w});
            adj[n.b].push_back({n.a, n.w});
        }
    }

    void parseStage1(const string &stage1file)
    {
        ifstream in(stage1file);
        if (!in)
            throw runtime_error("Cannot open stage1: " + stage1file);

        string tok;
        in >> tok;
        if (tok != "HPWL")
            throw runtime_error("stage1: expected HPWL");
        double dummy;
        in >> dummy;

        in >> tok;
        if (tok != "SOFTMODULE")
            throw runtime_error("stage1: expected SOFTMODULE");
        int n;
        in >> n;

        for (int i = 0; i < n; i++)
        {
            string nm;
            int k;
            in >> nm >> k;
            int id = name2id.at(nm);

            int minX = INT_MAX, minY = INT_MAX, maxX = INT_MIN, maxY = INT_MIN;
            for (int j = 0; j < k; j++)
            {
                int x, y;
                in >> x >> y;
                minX = min(minX, x);
                minY = min(minY, y);
                maxX = max(maxX, x);
                maxY = max(maxY, y);
            }

            // Convert corner bbox into half-open bbox.
            // If corners include (x+w, y+h), then half-open max is exactly that.
            mods[id].minx = minX;
            mods[id].miny = minY;
            mods[id].maxx = maxX;
            mods[id].maxy = maxY;

            int w = mods[id].maxx - mods[id].minx;
            int h = mods[id].maxy - mods[id].miny;
            if (w <= 0 || h <= 0)
                throw runtime_error("stage1: invalid rect for " + nm);
            mods[id].area = 1LL * w * h;
        }
    }

    // -------- geometry helpers --------
    inline double centerX(const Module &m) const { return 0.5 * (m.minx + m.maxx); }
    inline double centerY(const Module &m) const { return 0.5 * (m.miny + m.maxy); }

    double totalHPWL() const
    {
        double s = 0.0;
        for (auto &n : nets)
        {
            const auto &A = mods[n.a];
            const auto &B = mods[n.b];
            s += (double)n.w * (fabs(centerX(A) - centerX(B)) + fabs(centerY(A) - centerY(B)));
        }
        return s;
    }

    bool bboxLegal(const Module &m, int nminx, int nminy, int nmaxx, int nmaxy, long long narea) const
    {
        if (nminx < 0 || nminy < 0 || nmaxx > chipW || nmaxy > chipH)
            return false;
        int w = nmaxx - nminx;
        int h = nmaxy - nminy;
        if (w <= 0 || h <= 0)
            return false;

        double ar = (double)w / (double)h;
        if (ar < aspectMin || ar > aspectMax)
            return false;

        long long bboxArea = 1LL * w * h;
        double rr = (double)narea / (double)bboxArea;
        if (rr < rectRatioMin || rr > 1.0)
            return false;

        if (narea < m.minArea)
            return false;
        return true;
    }

    // -------- build grid + init frontier --------
    void buildGridAndFrontiers()
    {
        if (chipW <= 0 || chipH <= 0)
            throw runtime_error("Invalid CHIP size");
        grid.assign((size_t)chipW * (size_t)chipH, -1);

        auto paintRect = [&](int id, int minx, int miny, int maxx, int maxy)
        {
            for (int y = miny; y < maxy; y++)
            {
                size_t base = (size_t)y * (size_t)chipW;
                for (int x = minx; x < maxx; x++)
                {
                    int &cell = grid[base + (size_t)x];
                    if (cell != -1)
                    {
                        throw runtime_error("Overlap painting " + mods[id].name + " with " + mods[cell].name);
                    }
                    cell = id;
                }
            }
        };

        // paint fixed then soft
        for (auto &m : mods)
            if (m.type == ModType::FIXED)
                paintRect(m.id, m.minx, m.miny, m.maxx, m.maxy);
        for (auto &m : mods)
            if (m.type == ModType::SOFT)
                paintRect(m.id, m.minx, m.miny, m.maxx, m.maxy);

        // frontier bitsets
        inFrontier.assign(mods.size(), vector<uint8_t>((size_t)chipW * (size_t)chipH, 0));

        // init frontiers: all empty cells adjacent (4-neighbor) to module cells on bbox boundary.
        // For speed, we only scan bbox boundary of each soft module (rect initially).
        for (auto &m : mods)
        {
            if (m.type != ModType::SOFT)
                continue;
            m.frontier.clear();
            addFrontierFromBBoxBoundary(m);
        }
    }

    void frontierAdd(Module &m, int x, int y)
    {
        if (x < 0 || y < 0 || x >= chipW || y >= chipH)
            return;
        int p = packCell(x, y, chipW);
        if (grid[p] != -1)
            return;
        if (inFrontier[m.id][p])
            return;
        inFrontier[m.id][p] = 1;
        m.frontier.push_back(p);
    }

    void addFrontierFromBBoxBoundary(Module &m)
    {
        // left/right edges
        for (int y = m.miny; y < m.maxy; y++)
        {
            frontierAdd(m, m.minx - 1, y);
            frontierAdd(m, m.maxx, y);
        }
        // bottom/top edges
        for (int x = m.minx; x < m.maxx; x++)
        {
            frontierAdd(m, x, m.miny - 1);
            frontierAdd(m, x, m.maxy);
        }
    }

    // After adding a pixel to module, update frontier locally.
    void updateFrontierAfterAdd(Module &m, int px, int py)
    {
        // The added pixel itself should not remain frontier
        int p = packCell(px, py, chipW);
        inFrontier[m.id][p] = 0;

        // Add its empty neighbors
        frontierAdd(m, px - 1, py);
        frontierAdd(m, px + 1, py);
        frontierAdd(m, px, py - 1);
        frontierAdd(m, px, py + 1);
    }

    // -------- forces / priority --------
    void computeForces()
    {
        for (auto &m : mods)
        {
            m.fx = 0;
            m.fy = 0;
        }
        for (auto &n : nets)
        {
            auto &A = mods[n.a];
            auto &B = mods[n.b];
            double ax = centerX(A), ay = centerY(A);
            double bx = centerX(B), by = centerY(B);
            double dx = bx - ax, dy = by - ay;
            A.fx += n.w * dx;
            A.fy += n.w * dy;
            B.fx -= n.w * dx;
            B.fy -= n.w * dy;
        }
    }

    // ΔHPWL if module m bbox becomes (nminx,nminy,nmaxx,nmaxy)
    double deltaHPWL_bbox(const Module &m, int nminx, int nminy, int nmaxx, int nmaxy) const
    {
        double oldCx = centerX(m), oldCy = centerY(m);
        double newCx = 0.5 * (nminx + nmaxx), newCy = 0.5 * (nminy + nmaxy);

        double d = 0.0;
        for (const auto &e : adj[m.id])
        {
            const Module &o = mods[e.to];
            double ox = centerX(o), oy = centerY(o);
            double old = fabs(oldCx - ox) + fabs(oldCy - oy);
            double neu = fabs(newCx - ox) + fabs(newCy - oy);
            d += (double)e.w * (neu - old);
        }
        return d;
    }

    // Candidate evaluation for adding pixel (x,y) to module m
    bool canAddPixel(const Module &m, int x, int y) const
    {
        if (x < 0 || y < 0 || x >= chipW || y >= chipH)
            return false;
        int p = packCell(x, y, chipW);
        if (grid[p] != -1)
            return false;

        // Must be 4-neighbor adjacent to module to preserve connectivity
        // (Frontier is built that way, but re-check is cheap and safer.)
        bool adjacent = false;
        if (x > 0 && grid[p - 1] == m.id)
            adjacent = true;
        if (x + 1 < chipW && grid[p + 1] == m.id)
            adjacent = true;
        if (y > 0 && grid[p - chipW] == m.id)
            adjacent = true;
        if (y + 1 < chipH && grid[p + chipW] == m.id)
            adjacent = true;
        if (!adjacent)
            return false;

        // compute new bbox
        int nminx = min(m.minx, x);
        int nminy = min(m.miny, y);
        int nmaxx = max(m.maxx, x + 1);
        int nmaxy = max(m.maxy, y + 1);
        long long narea = m.area + 1;

        return bboxLegal(m, nminx, nminy, nmaxx, nmaxy, narea);
    }

    // Apply pixel add
    void applyAddPixel(Module &m, int x, int y)
    {
        int p = packCell(x, y, chipW);
        grid[p] = m.id;
        m.area += 1;
        m.minx = min(m.minx, x);
        m.miny = min(m.miny, y);
        m.maxx = max(m.maxx, x + 1);
        m.maxy = max(m.maxy, y + 1);
        updateFrontierAfterAdd(m, x, y);
    }

    // Choose best candidate from frontier bag (lazy cleanup of stale entries)
    bool expandOneStep(Module &m)
    {
        if (m.frontier.empty())
            return false;

        double mag = hypot(m.fx, m.fy);
        double dx = (mag > 1e-12) ? m.fx / mag : 0.0;
        double dy = (mag > 1e-12) ? m.fy / mag : 0.0;

        int bestIdx = -1;
        double bestScore = -1e300;
        double bestDHP = 0.0;
        int bestX = 0, bestY = 0;
        int bestMinx = 0, bestMiny = 0, bestMaxx = 0, bestMaxy = 0;

        // Scan frontier bag (can include stale entries; skip them)
        for (int i = (int)m.frontier.size() - 1; i >= 0; i--)
        {
            int p = m.frontier[i];

            // If it's no longer marked frontier, it's stale
            if (!inFrontier[m.id][p])
                continue;

            // If it is occupied now, stale => unmark
            if (grid[p] != -1)
            {
                inFrontier[m.id][p] = 0;
                continue;
            }

            int x = cellX(p, chipW), y = cellY(p, chipW);

            if (!canAddPixel(m, x, y))
            {
                // keep it frontier for later? usually no, because legality depends on bbox and may change.
                // We'll keep it; legality can become true later.
                continue;
            }

            // new bbox
            int nminx = min(m.minx, x);
            int nminy = min(m.miny, y);
            int nmaxx = max(m.maxx, x + 1);
            int nmaxy = max(m.maxy, y + 1);

            double dHP = deltaHPWL_bbox(m, nminx, nminy, nmaxx, nmaxy);

            // direction dot: from current bbox center to the new pixel
            double cx = centerX(m), cy = centerY(m);
            double vpx = (x + 0.5) - cx;
            double vpy = (y + 0.5) - cy;
            double vmag = hypot(vpx, vpy);
            double dirDot = 0.0;
            if (vmag > 1e-12)
                dirDot = (vpx / vmag) * dx + (vpy / vmag) * dy;

            // score: prefer more HPWL decrease, and slight preference to direction
            // minimize dHP; so score uses -dHP
            double score = (-dHP) + dirBias * dirDot;

            if (score > bestScore)
            {
                bestScore = score;
                bestIdx = i;
                bestDHP = dHP;
                bestX = x;
                bestY = y;
                bestMinx = nminx;
                bestMiny = nminy;
                bestMaxx = nmaxx;
                bestMaxy = nmaxy;
            }
        }

        if (bestIdx < 0)
            return false;

        // Accept rule
        if (bestDHP < 0.0 || (allowNeutral && bestDHP == 0.0) || bestDHP <= hpwlEps)
        {
            applyAddPixel(m, bestX, bestY);
            return true;
        }
        return false;
    }

    // -------- optimize loop --------
    void optimize()
    {
        for (int r = 1; r <= maxRounds; r++)
        {
            cout << "Round " << r << "/" << maxRounds << "\n";
            computeForces();
            double cur = totalHPWL();
            cout << "Refinement round " << r << "/" << maxRounds
                 << ", current HPWL = " << scientific << setprecision(6) << cur << "\n";

            // order by force magnitude
            vector<int> order;
            for (auto &m : mods)
                if (m.type == ModType::SOFT)
                    order.push_back(m.id);
            sort(order.begin(), order.end(), [&](int a, int b)
                 {
        double ma=hypot(mods[a].fx,mods[a].fy);
        double mb=hypot(mods[b].fx,mods[b].fy);
        return ma>mb; });

            long long roundAdds = 0;
            for (int id : order)
            {
                Module &m = mods[id];

                int adds = 0;
                for (int step = 0; step < maxStepsPerModPerRound; step++)
                {
                    if (!expandOneStep(m))
                        break;
                    adds++;
                }
                if (adds > 0)
                {
                    roundAdds += adds;
                    cout << "  Module " << m.name << " expanded by " << adds << " pixels\n";
                }
            }

            if (roundAdds == 0)
            {
                cout << "  No expansions accepted this round; stopping early.\n";
                break;
            }
        }
    }

    // -------- polygon output from occupied cells --------
    // Build boundary edges for a module from grid cells within bbox, then stitch into one cycle.
    vector<pair<int, int>> extractPolygon(const Module &m) const
    {
        // Collect directed boundary edges along grid lines.
        // Each occupied cell contributes edges where neighbor is not same module.
        // We'll store edges as (from)->(to) with integer vertices.
        struct DirEdge
        {
            int x1, y1, x2, y2;
            int dir;
        }; // dir:0E 1N 2W 3S
        vector<DirEdge> edges;
        edges.reserve((size_t)m.area * 2);

        auto isOcc = [&](int x, int y) -> bool
        {
            if (x < 0 || y < 0 || x >= chipW || y >= chipH)
                return false;
            return grid[packCell(x, y, chipW)] == m.id;
        };

        for (int y = m.miny; y < m.maxy; y++)
        {
            for (int x = m.minx; x < m.maxx; x++)
            {
                if (!isOcc(x, y))
                    continue;

                // cell square corners: (x,y) to (x+1,y+1)
                // Left side boundary if (x-1,y) not occ
                if (!isOcc(x - 1, y))
                {
                    edges.push_back({x, y, x, y + 1, 1}); // up along left side => dir=N
                }
                // Right side boundary if (x+1,y) not occ
                if (!isOcc(x + 1, y))
                {
                    edges.push_back({x + 1, y + 1, x + 1, y, 3}); // down along right side => dir=S (to keep CCW)
                }
                // Bottom boundary if (x,y-1) not occ
                if (!isOcc(x, y - 1))
                {
                    edges.push_back({x + 1, y, x, y, 2}); // west along bottom => dir=W (CCW)
                }
                // Top boundary if (x,y+1) not occ
                if (!isOcc(x, y + 1))
                {
                    edges.push_back({x, y + 1, x + 1, y + 1, 0}); // east along top => dir=E
                }
            }
        }

        if (edges.empty())
        {
            // fallback: bbox rectangle
            return {
                {m.minx, m.miny}, {m.minx, m.maxy}, {m.maxx, m.maxy}, {m.maxx, m.miny}};
        }

        // Build outgoing edge lists keyed by start vertex
        unordered_map<long long, vector<int>> out;
        out.reserve(edges.size() * 2);

        long long startKey = LLONG_MAX;
        int startEdge = -1;

        for (int i = 0; i < (int)edges.size(); i++)
        {
            auto &e = edges[i];
            long long k = keyVertex(e.x1, e.y1);
            out[k].push_back(i);

            // choose lexicographically smallest start vertex as starting point
            // to get deterministic polygon
            long long lex = ((long long)e.y1 << 32) ^ (unsigned int)e.x1;
            if (lex < startKey)
            {
                startKey = lex;
                startEdge = i;
            }
        }

        // Helper: direction from edge endpoints (we already store dir)
        auto turnCost = [&](int curDir, int nextDir) -> int
        {
            // prefer left turns to keep CCW boundary:
            // define cost: straight(0), left(1), right(2), back(3) in preference order
            // With dir coding E(0),N(1),W(2),S(3)
            int d = (nextDir - curDir + 4) % 4;
            // d==0 straight, d==1 left, d==3 right, d==2 back
            if (d == 0)
                return 0;
            if (d == 1)
                return 1;
            if (d == 3)
                return 2;
            return 3;
        };

        // Traverse with left-hand rule.
        vector<pair<int, int>> poly;
        poly.reserve(edges.size() + 1);

        int curEdge = startEdge;
        int curDir = edges[curEdge].dir;
        int sx = edges[curEdge].x1, sy = edges[curEdge].y1;
        int cx = sx, cy = sy;

        // add first vertex
        poly.push_back({cx, cy});

        // walk
        int safe = 0;
        while (true)
        {
            auto &e = edges[curEdge];
            int nx = e.x2, ny = e.y2;
            cx = nx;
            cy = ny;
            poly.push_back({cx, cy});

            if (cx == sx && cy == sy)
                break;

            long long k = keyVertex(cx, cy);
            auto it = out.find(k);
            if (it == out.end())
            {
                break; // broken boundary
            }
            // choose next edge by minimal turn cost; if tie, deterministic by next vertex
            int best = -1;
            int bestC = 999;
            int bestNextY = INT_MAX, bestNextX = INT_MAX;

            for (int idx : it->second)
            {
                auto &ne = edges[idx];
                int cost = turnCost(curDir, ne.dir);
                // next endpoint for tie-break
                int tx = ne.x2, ty = ne.y2;
                if (cost < bestC || (cost == bestC && (ty < bestNextY || (ty == bestNextY && tx < bestNextX))))
                {
                    bestC = cost;
                    best = idx;
                    bestNextX = tx;
                    bestNextY = ty;
                }
            }
            if (best < 0)
                break;
            curEdge = best;
            curDir = edges[curEdge].dir;

            if (++safe > (int)edges.size() + 10)
                break;
        }

        // Remove last duplicated start if present
        if (poly.size() >= 2 && poly.back() == poly.front())
            poly.pop_back();

        // Compress collinear points
        vector<pair<int, int>> simp;
        simp.reserve(poly.size());
        auto collinear = [&](pair<int, int> a, pair<int, int> b, pair<int, int> c)
        {
            // axis-aligned: collinear if (a,b,c share x) or (a,b,c share y)
            return (a.first == b.first && b.first == c.first) || (a.second == b.second && b.second == c.second);
        };

        for (auto &pt : poly)
        {
            simp.push_back(pt);
            while (simp.size() >= 3)
            {
                auto a = simp[simp.size() - 3];
                auto b = simp[simp.size() - 2];
                auto c = simp[simp.size() - 1];
                if (collinear(a, b, c))
                {
                    // remove middle
                    simp[simp.size() - 2] = simp.back();
                    simp.pop_back();
                }
                else
                    break;
            }
        }

        // Ensure at least 4 points
        if (simp.size() < 4)
        {
            return {
                {m.minx, m.miny}, {m.minx, m.maxy}, {m.maxx, m.maxy}, {m.maxx, m.miny}};
        }
        return simp;
    }

    void writeOutput(const string &outFile)
    {
        ofstream out(outFile);
        if (!out)
            throw runtime_error("Cannot write output: " + outFile);

        out << fixed << setprecision(1);
        out << "HPWL " << totalHPWL() << "\n";

        int softN = 0;
        for (auto &m : mods)
            if (m.type == ModType::SOFT)
                softN++;
        out << "SOFTMODULE " << softN << "\n";

        for (auto &m : mods)
        {
            if (m.type != ModType::SOFT)
                continue;
            auto poly = extractPolygon(m);
            out << m.name << " " << (int)poly.size() << "\n";
            for (auto &p : poly)
            {
                out << p.first << " " << p.second << "\n";
            }
        }
    }
};

int main(int argc, char **argv)
{
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    if (argc < 4)
    {
        cerr << "Usage: ./refiner_pixel <input> <stage1_out> <final_out>\n";
        return 1;
    }

    try
    {
        RefinerPixel r;
        r.parseProblem(argv[1]);
        r.parseStage1(argv[2]);
        r.buildGridAndFrontiers();
        r.optimize();
        r.writeOutput(argv[3]);
    }
    catch (const exception &e)
    {
        cerr << "Error: " << e.what() << "\n";
        return 2;
    }

    return 0;
}
