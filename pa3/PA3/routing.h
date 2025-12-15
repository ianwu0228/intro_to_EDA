#ifndef ROUTING_H
#define ROUTING_H

#include <vector>
#include <string>
#include <iostream>
#include <fstream>
#include <set>
#include <algorithm>

using namespace std;

const int CELL_EMPTY = -1;
const int CELL_BLOCK = -2;

struct Point {
    int x, y;
    bool operator==(const Point& other) const { return x == other.x && y == other.y; }
    bool operator!=(const Point& other) const { return !(*this == other); }
    bool operator<(const Point& other) const { return x != other.x ? x < other.x : y < other.y; }
};

struct Net {
    int id;
    string name;
    Point source, target;
    vector<Point> routed_path;
    bool is_routed = false;

    Net() = default;
    Net(int id, string name, Point s, Point t) 
        : id(id), name(name), source(s), target(t), is_routed(false) {}

    int hpwl() const { return abs(source.x - target.x) + abs(source.y - target.y); }
};

class Maze {
public:
    int rows, cols;
    vector<vector<int>> grid_state;
    vector<vector<int>> pin_grid;
    vector<vector<int>> history_grid; // congestion costs
    vector<Net> nets;

    Maze(int r, int c);
    void add_block(int lx, int rx, int ly, int ry);
    void add_net(int id, string name, int sx, int sy, int tx, int ty);
    void init_pins();

    bool route_net_a_star(int net_idx); // to do
    set<int> route_force(int net_idx);  // to do
    void rip_up_net(int net_idx);   // to do
    void commit_net(int net_idx);   // to do
    bool is_valid(int x, int y);

private:
    int manhattan_dist(Point p1, Point p2);
};

void parse_input(ifstream& fin, int& rows, int& cols, Maze*& maze);
void output(ofstream& fout, const Maze* maze);

#endif // ROUTING_H