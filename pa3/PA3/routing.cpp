#include "routing.h"
#include <queue>
#include <map>
#include <limits>

Maze::Maze(int r, int c) : rows(r), cols(c)
{
    grid_state.assign(cols, vector<int>(rows, CELL_EMPTY));
    pin_grid.assign(cols, vector<int>(rows, CELL_EMPTY));
    history_grid.assign(cols, vector<int>(rows, 0));
}

void Maze::add_block(int lx, int rx, int ly, int ry)
{
    for (int x = min(lx, rx); x <= max(lx, rx); ++x)
    {
        for (int y = min(ly, ry); y <= max(ly, ry); ++y)
        {
            if (is_valid(x, y))
                grid_state[x][y] = CELL_BLOCK;
        }
    }
}

void Maze::add_net(int id, string name, int sx, int sy, int tx, int ty)
{
    nets.push_back(Net(id, name, {sx, sy}, {tx, ty}));
}

void Maze::init_pins()
{
    for (const auto &net : nets)
    {
        if (is_valid(net.source.x, net.source.y))
        {
            pin_grid[net.source.x][net.source.y] = net.id;
            grid_state[net.source.x][net.source.y] = net.id;
        }
        if (is_valid(net.target.x, net.target.y))
        {
            pin_grid[net.target.x][net.target.y] = net.id;
            grid_state[net.target.x][net.target.y] = net.id;
        }
    }
}

bool Maze::is_valid(int x, int y)
{
    return x >= 0 && x < cols && y >= 0 && y < rows;
}

int Maze::manhattan_dist(Point p1, Point p2)
{
    return abs(p1.x - p2.x) + abs(p1.y - p2.y);
}

struct Node
{
    Point p;
    int g, h;
    int f() const { return g + h; }
    bool operator>(const Node &other) const { return f() > other.f(); }
};

// bool Maze::route_net_a_star(int net_idx)
// {

//     /*
//     TODO : Implement standard A* search.

//     1. Initialize priority queue with source node.
//     2. explore neighbors (up, down, left, right).
//     3. STRICT LEGALITY CHECK: Only allow moves to neighbors that are:
//         a) Inside grid boundaries.
//         b) NOT occupied by static blocks (CELL_BLOCK).
//         c) NOT occupied by other nets (grid_state must be CELL_EMPTY).
//         Exception: The neighbor is the target pin of this net.
//     4. If target reached, reconstruct path by backtracking from target to source.
//     5. Return true if a path is found, false otherwise.
//     */
// }

bool Maze::route_net_a_star(int net_idx)
{

    // Standard A* search with strict legality (no overlap with other nets or blocks)
    if (net_idx < 0 || net_idx >= (int)nets.size())
        return false;

    const Net &net = nets[net_idx];
    Point s = net.source;
    Point t = net.target;

    // Clear any previous path for this net (do not touch grid_state here)
    nets[net_idx].routed_path.clear();

    // Priority queue ordered by smallest f = g + h
    std::priority_queue<Node, std::vector<Node>, std::greater<Node>> pq;

    const int INF = std::numeric_limits<int>::max();
    std::vector<std::vector<int>> g_cost(cols, std::vector<int>(rows, INF));
    std::vector<std::vector<bool>> closed(cols, std::vector<bool>(rows, false));
    std::vector<std::vector<Point>> parent(cols, std::vector<Point>(rows, Point{-1, -1}));

    if (!is_valid(s.x, s.y) || !is_valid(t.x, t.y))
        return false;

    Node start{s, 0, manhattan_dist(s, t)};
    pq.push(start);
    g_cost[s.x][s.y] = 0;

    const int dx[4] = {1, -1, 0, 0};
    const int dy[4] = {0, 0, 1, -1};

    bool reached = false;

    while (!pq.empty())
    {
        Node cur = pq.top();
        pq.pop();
        Point p = cur.p;

        if (closed[p.x][p.y])
            continue;
        closed[p.x][p.y] = true;

        if (p == t)
        {
            reached = true;
            break;
        }

        for (int dir = 0; dir < 4; ++dir)
        {
            int nx = p.x + dx[dir];
            int ny = p.y + dy[dir];

            if (!is_valid(nx, ny))
                continue;

            // Static block cannot be passed
            if (grid_state[nx][ny] == CELL_BLOCK)
                continue;

            // Strict legality: cannot step onto other nets,
            // except this net's own source/target pins.
            int cell_owner = grid_state[nx][ny];
            if (cell_owner != CELL_EMPTY)
            {
                if (!((nx == s.x && ny == s.y) || (nx == t.x && ny == t.y)))
                {
                    // occupied by another net (or unexpected pin) – illegal
                    continue;
                }
            }

            int tentative_g = g_cost[p.x][p.y] + 1; // base cost = 1 per step
            if (tentative_g < g_cost[nx][ny])
            {
                g_cost[nx][ny] = tentative_g;
                parent[nx][ny] = p;
                Node nxt{Point{nx, ny}, tentative_g,
                         manhattan_dist(Point{nx, ny}, t)};
                pq.push(nxt);
            }
        }
    }

    if (!reached)
    {
        // No path found
        nets[net_idx].routed_path.clear();
        return false;
    }

    // Reconstruct path from target back to source
    std::vector<Point> rev_path;
    Point cur = t;
    rev_path.push_back(cur);
    while (!(cur == s))
    {
        Point par = parent[cur.x][cur.y];
        if (par.x == -1 && par.y == -1)
        {
            // Should not happen if reached is true, but guard anyway
            nets[net_idx].routed_path.clear();
            return false;
        }
        cur = par;
        rev_path.push_back(cur);
    }

    // Reverse to get source -> target
    std::reverse(rev_path.begin(), rev_path.end());
    nets[net_idx].routed_path = rev_path;

    return true;
}

// set<int> Maze::route_force(int net_idx)
// {
//     /*
//     TODO : Implement "Force" A* search to identify congestion.
//         This is similar to standard A*, but with relaxed constraints and penalties.
//         This function MUST find a path, even if it overlaps with existing nets.

//     1. Allow moves into cells occupied by other nets.
//     2. COST CALCULATION:
//         The cost of stepping to a neighbor (g_score) must include:
//         a) Base cost.
//         b) HISTORY COST: Add 'history_grid[x][y]' to the step cost.
//             (This is crucial for convergence; it makes frequently contended
//             cells permanently more expensive).
//         c) COLLISION PENALTY: If the cell is currently occupied by another net,
//         add a penalty to prefer empty cells first.
//     3. Collect Victims:
//         - Once the target is reached, retrace the path.
//         - Any cell on this path owned by ANOTHER net represents a collision.
//         - Add the ID of that "victim" net to a set.
//     4. Update the net's path with this new forced path.
//     5. Return the set of victim net IDs to be ripped up by the main loop.
//     */
// }

set<int> Maze::route_force(int net_idx)
{
    // Relaxed A* that allows overlaps with other nets, with penalties
    set<int> victims;
    if (net_idx < 0 || net_idx >= (int)nets.size())
        return victims;

    const Net &net = nets[net_idx];
    Point s = net.source;
    Point t = net.target;

    nets[net_idx].routed_path.clear();

    const int INF = std::numeric_limits<int>::max();
    const int BASE_COST = 1;
    const int COLLISION_PENALTY = 20; // favor empty cells over overlaps

    std::priority_queue<Node, std::vector<Node>, std::greater<Node>> pq;
    std::vector<std::vector<int>> g_cost(cols, std::vector<int>(rows, INF));
    std::vector<std::vector<bool>> closed(cols, std::vector<bool>(rows, false));
    std::vector<std::vector<Point>> parent(cols, std::vector<Point>(rows, Point{-1, -1}));

    if (!is_valid(s.x, s.y) || !is_valid(t.x, t.y))
        return victims;

    Node start{s, 0, manhattan_dist(s, t)};
    pq.push(start);
    g_cost[s.x][s.y] = 0;

    const int dx[4] = {1, -1, 0, 0};
    const int dy[4] = {0, 0, 1, -1};

    bool reached = false;

    while (!pq.empty())
    {
        Node cur = pq.top();
        pq.pop();
        Point p = cur.p;

        if (closed[p.x][p.y])
            continue;
        closed[p.x][p.y] = true;

        if (p == t)
        {
            reached = true;
            break;
        }

        for (int dir = 0; dir < 4; ++dir)
        {
            int nx = p.x + dx[dir];
            int ny = p.y + dy[dir];

            if (!is_valid(nx, ny))
                continue;

            // Blocks still cannot be passed
            if (grid_state[nx][ny] == CELL_BLOCK)
                continue;

            int step_cost = BASE_COST + history_grid[nx][ny];

            int cell_owner = grid_state[nx][ny];
            if (cell_owner >= 0 &&
                cell_owner != nets[net_idx].id &&
                !(nx == t.x && ny == t.y))
            {
                // Moving through another net => collision, add penalty
                step_cost += COLLISION_PENALTY;
            }

            int tentative_g = g_cost[p.x][p.y] + step_cost;
            if (tentative_g < g_cost[nx][ny])
            {
                g_cost[nx][ny] = tentative_g;
                parent[nx][ny] = p;
                Node nxt{Point{nx, ny}, tentative_g,
                         manhattan_dist(Point{nx, ny}, t)};
                pq.push(nxt);
            }
        }
    }

    if (!reached)
    {
        // Failed to find any path (should be rare unless totally blocked by static obstacles)
        nets[net_idx].routed_path.clear();
        return victims;
    }

    // Reconstruct forced path and collect victims & update history
    std::vector<Point> rev_path;
    Point cur = t;
    rev_path.push_back(cur);
    while (!(cur == s))
    {
        Point par = parent[cur.x][cur.y];
        if (par.x == -1 && par.y == -1)
        {
            nets[net_idx].routed_path.clear();
            victims.clear();
            return victims;
        }
        cur = par;
        rev_path.push_back(cur);
    }

    std::reverse(rev_path.begin(), rev_path.end());

    // Scan path to identify victim nets and update history costs
    for (const Point &p : rev_path)
    {
        if (!(p == s) && !(p == t))
        {
            // Increase history cost so this cell becomes more expensive next time
            history_grid[p.x][p.y] += 1;
        }
        int owner = grid_state[p.x][p.y];
        if (owner >= 0 && owner != nets[net_idx].id)
        {
            victims.insert(owner);
        }
    }

    nets[net_idx].routed_path = rev_path;

    return victims;
}

// void Maze::commit_net(int net_idx)
// {
//     /*
//     TODO : Commit the temporary path to the main grid.
//         This function is called after a successful route (standard or forced)
//         to mark the cells as occupied by this net.

//     1. Iterate through every Point in the net's 'routed_path'.
//     2. Update the 'grid_state' at each coordinate to match this net's ID.
//         (grid_state[p.x][p.y] = nets[net_idx].id)
//     3. Mark the net as routed (nets[net_idx].is_routed = true).
//     */
// }

void Maze::commit_net(int net_idx)
{
    // Commit this net's routed path to the grid_state
    if (net_idx < 0 || net_idx >= (int)nets.size())
        return;

    int net_id = nets[net_idx].id;

    for (const Point &p : nets[net_idx].routed_path)
    {
        if (!is_valid(p.x, p.y))
            continue;
        grid_state[p.x][p.y] = net_id;
    }

    nets[net_idx].is_routed = true;
}

// void Maze::rip_up_net(int net_idx)
// {
//     /*
//     TODO : Remove a net's path from the grid.
//     1. Iterate through every Point in the net's 'routed_path'.
//     2. For each point (x, y):
//         - Check if this cell is currently owned by this net (grid_state[x][y] == net_id).
//         - If yes, reset it.
//         - Do not clear fixed pins.
//         If pin_grid[x][y] has a pin, set grid_state back to that pin ID.
//         Otherwise, set it to CELL_EMPTY.
//     3. Clear the 'routed_path' vector and set 'is_routed' to false.
//     */
// }

void Maze::rip_up_net(int net_idx)
{
    // Remove this net's path from grid_state while keeping pins intact
    if (net_idx < 0 || net_idx >= (int)nets.size())
        return;

    int net_id = nets[net_idx].id;

    for (const Point &p : nets[net_idx].routed_path)
    {
        if (!is_valid(p.x, p.y))
            continue;

        // Only clear cells that currently belong to this net
        if (grid_state[p.x][p.y] == net_id)
        {
            if (pin_grid[p.x][p.y] != CELL_EMPTY)
            {
                // Restore pin information (usually same as net_id)
                grid_state[p.x][p.y] = pin_grid[p.x][p.y];
            }
            else
            {
                grid_state[p.x][p.y] = CELL_EMPTY;
            }
        }
    }

    nets[net_idx].routed_path.clear();
    nets[net_idx].is_routed = false;
}

void parse_input(ifstream &fin, int &rows, int &cols, Maze *&maze)
{
    string type;
    int num;
    fin >> type >> rows;
    fin >> type >> cols;
    if (!maze)
        maze = new Maze(rows, cols);

    fin >> type >> num;
    for (int i = 0; i < num; ++i)
    {
        Point p1, p2;
        fin >> p1.x >> p2.x >> p1.y >> p2.y;
        maze->add_block(p1.x, p2.x, p1.y, p2.y);
    }

    fin >> type >> num;
    for (int i = 0; i < num; ++i)
    {
        string name;
        Point s, t;
        fin >> name >> s.x >> s.y >> t.x >> t.y;
        maze->add_net(i, name, s.x, s.y, t.x, t.y);
    }
}

void output(ofstream &fout, const Maze *maze)
{
    for (const auto &net : maze->nets)
    {
        if (net.is_routed && !net.routed_path.empty())
        {
            fout << net.name << " " << (net.routed_path.size() - 2) << "\nbegin\n";
            Point s = net.routed_path[0];
            int dx = net.routed_path[1].x - s.x;
            int dy = net.routed_path[1].y - s.y;
            for (size_t i = 2; i < net.routed_path.size(); ++i)
            {
                int ndx = net.routed_path[i].x - net.routed_path[i - 1].x;
                int ndy = net.routed_path[i].y - net.routed_path[i - 1].y;
                if (ndx != dx || ndy != dy)
                {
                    fout << s.x << " " << s.y << " " << net.routed_path[i - 1].x << " " << net.routed_path[i - 1].y << "\n";
                    s = net.routed_path[i - 1];
                    dx = ndx;
                    dy = ndy;
                }
            }
            fout << s.x << " " << s.y << " " << net.routed_path.back().x << " " << net.routed_path.back().y << "\nend\n";
        }
        else
        {
            fout << net.name << " FAILED\n";
        }
    }
}