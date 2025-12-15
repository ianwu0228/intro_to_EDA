#include "routing.h"
#include <deque>
#include <chrono>
#include <numeric> // for iota
#include <climits> // for LLONG_MAX

using namespace std;

// Compute total grid usage = sum over nets of (routed_path.size() - 2)
long long compute_total_grid_usage(const Maze &maze)
{
    long long total = 0;
    for (const auto &net : maze.nets)
    {
        if (!net.is_routed || net.routed_path.size() < 2)
            return LLONG_MAX; // treat as invalid solution

        // grid usage definition in slides: path length - 2 (exclude pins)
        total += static_cast<long long>(net.routed_path.size()) - 2;
    }
    return total;
}

// Route all nets in a given order, using your existing strategy
// (A* first, then force routing + rip-up & reroute).
// Returns true only if ALL nets are routed legally.
bool route_all_nets_in_order(Maze &maze, const vector<int> &order)
{
    deque<int> route_queue(order.begin(), order.end());

    while (!route_queue.empty())
    {
        int net_idx = route_queue.front();
        route_queue.pop_front();

        // 1. Try standard legal routing
        if (maze.route_net_a_star(net_idx))
        {
            maze.commit_net(net_idx);
        }
        else
        {
            // 2. Failed: Force route, identify victims, and increase history costs
            set<int> victims_ids = maze.route_force(net_idx);

            // If force failed (e.g. completely walled off), fail this permutation
            if (maze.nets[net_idx].routed_path.empty())
            {
                return false;
            }

            // 3. Rip up all victims so we can commit the forced path legally
            for (int v_id : victims_ids)
            {
                if (maze.nets[v_id].is_routed)
                {
                    maze.rip_up_net(v_id);
                    route_queue.push_back(v_id);
                }
            }
            maze.commit_net(net_idx);
        }
    }

    // Verify that every net is routed
    for (const auto &net : maze.nets)
    {
        if (!net.is_routed || net.routed_path.empty())
            return false;
    }
    return true;
}

int main(int argc, char *argv[])
{
    int rows = 0, cols = 0;
    Maze *maze = nullptr;

    ifstream fin;
    ofstream fout;
    fin.open(argv[1]);
    fout.open(argv[2]);

    parse_input(fin, rows, cols, maze);
    maze->init_pins();

    const int N = static_cast<int>(maze->nets.size());
    if (N == 0)
    {
        // Nothing to route
        output(fout, maze);
        delete maze;
        return 0;
    }

    // Start timing
    using Clock = std::chrono::steady_clock;
    auto start_time = Clock::now();
    const double TIME_LIMIT_SEC = 110.0; // 1 min 50 sec

    // ---------- 1. Build HPWL-sorted base order ----------
    vector<int> hpwl_order(N);
    iota(hpwl_order.begin(), hpwl_order.end(), 0);
    sort(hpwl_order.begin(), hpwl_order.end(),
         [&](int a, int b)
         { return maze->nets[a].hpwl() < maze->nets[b].hpwl(); });

    // Snapshot the initial empty maze state (blocks + pins only)
    Maze original = *maze;

    // The best solution we have found so far
    Maze *best_maze = new Maze(original);
    long long best_cost = LLONG_MAX;
    bool found_any = false;

    // We will permute POSITION indices [0..N-1],
    // and map them through hpwl_order to get actual net indices.
    vector<int> perm_pos(N);
    iota(perm_pos.begin(), perm_pos.end(), 0);

    // Enumerate permutations until time limit
    bool first_perm = true;
    do
    {
        auto now = Clock::now();
        double elapsed =
            std::chrono::duration<double>(now - start_time).count();
        if (elapsed > TIME_LIMIT_SEC)
            break;

        // Map current permutation of positions to actual net routing order
        vector<int> order(N);
        for (int i = 0; i < N; ++i)
            order[i] = hpwl_order[perm_pos[i]];

        // For the very first permutation, this is exactly HPWL order.
        // Create a fresh copy of the original maze
        Maze candidate = original;

        // Route according to this order
        if (!route_all_nets_in_order(candidate, order))
            continue; // this permutation failed, skip

        long long cost = compute_total_grid_usage(candidate);
        if (!found_any || cost < best_cost)
        {
            best_cost = cost;
            *best_maze = candidate;
            found_any = true;
        }

        first_perm = false;
    } while (std::next_permutation(perm_pos.begin(), perm_pos.end()));

    // Safety fallback: if for some reason no permutation produced
    // a valid full routing, try HPWL order once.
    if (!found_any)
    {
        *best_maze = original;
        vector<int> base_order = hpwl_order;
        route_all_nets_in_order(*best_maze, base_order);
    }

    // Output the best routing we have
    output(fout, best_maze);

    delete best_maze;
    delete maze;
    return 0;
}
