#ifndef MODULE_H
#define MODULE_H

#include <vector>
#include <string>
#include <algorithm> // for std::max, std::min
#include <cmath>     // for std::sqrt
#include <limits>
#include "node.h" // Node

using namespace std;

class Terminal
{
public:
    // constructor and destructor
    Terminal(string &name, size_t x, size_t y) : _name(name), _x1(x), _y1(y), _x2(x), _y2(y) {}
    virtual ~Terminal() {} // Virtual destructor is safer for inheritance

    // basic access methods
    string getName() const { return _name; } // Added const
    size_t getX1() const { return _x1; }
    size_t getX2() const { return _x2; }
    size_t getY1() const { return _y1; }
    size_t getY2() const { return _y2; }

    // Center coordinates for HPWL calculation
    size_t getCenterX() const { return (_x1 + _x2) / 2; }
    size_t getCenterY() const { return (_y1 + _y2) / 2; }

    // set functions
    void setName(string &name) { _name = name; }
    void setPos(size_t x1, size_t y1, size_t x2, size_t y2)
    {
        _x1 = x1;
        _y1 = y1;
        _x2 = x2;
        _y2 = y2;
    }
    void setID(size_t id) { _id = id; }

protected:
    string _name; // module name
    size_t _x1;   // min x coordinate of the terminal
    size_t _y1;   // min y coordinate of the terminal
    size_t _x2;   // max x coordinate of the terminal
    size_t _y2;   // max y coordinate of the terminal
    size_t _id;
};

// class Block : public Terminal
// {
// public:
//     // Constructor for SOFT modules (Area is known, Dimensions flexible)
//     Block(string &name, size_t minArea) : Terminal(name, 0, 0), _minArea(minArea), _isFixed(false)
//     {
//         // Default to square shape initially
//         _w = static_cast<size_t>(sqrt(minArea));
//         _h = _w;
//         if (_w * _h < _minArea)
//             _h++; // Ensure area constraint
//     }

//     // Constructor for FIXED modules (Dimensions and Position known)
//     Block(string &name, size_t w, size_t h, size_t x, size_t y) : Terminal(name, x, y), _w(w), _h(h), _minArea(w * h), _isFixed(true)
//     {
//         _x2 = _x1 + _w;
//         _y2 = _y1 + _h;
//     }

//     ~Block() {}

//     // basic access methods
//     size_t getWidth(bool rotate = false) const { return rotate ? _h : _w; }
//     size_t getHeight(bool rotate = false) const { return rotate ? _w : _h; }
//     size_t getArea() const { return _h * _w; }
//     size_t getMinArea() const { return _minArea; }
//     bool isFixed() const { return _isFixed; }

//     // set functions
//     void setWidth(size_t w) { _w = w; }
//     void setHeight(size_t h) { _h = h; }
//     void setID(size_t id) { _id = id; }

//     // Resize function for Soft Modules (Aspect Ratio = H / W)
//     void resize(double aspectRatio)
//     {
//         if (_isFixed)
//             return;

//         // Calculate W based on Area and AR: W = sqrt(Area / AR)
//         _w = static_cast<size_t>(std::sqrt(_minArea / aspectRatio));
//         // Calculate H based on W
//         if (_w == 0)
//             _w = 1; // Prevent div by zero
//         _h = static_cast<size_t>(std::ceil((double)_minArea / _w));

//         // Double check area constraint (integer math can be tricky)
//         if (_w * _h < _minArea)
//             _h++;
//     }

//     // other member functions
//     void setNode(Node *node) { _node = node; }
//     Node *getNode() { return _node; }

// private:
//     size_t _w;       // width of the block
//     size_t _h;       // height of the block
//     size_t _minArea; // NEW: Required minimum area
//     bool _isFixed;   // NEW: Flag for fixed modules
//     size_t _id;      // id of the block
//     Node *_node;     // pointer to the parent node
// };

class Block : public Terminal
{
public:
    // Constructor for SOFT modules (Area is known, Dimensions flexible)
    // ADDED: isGhost parameter (defaults to false)
    Block(string &name, size_t minArea, bool isGhost = false)
        : Terminal(name, 0, 0), _minArea(minArea), _isFixed(false), _isGhost(isGhost)
    {
        _w = static_cast<size_t>(sqrt(minArea));
        _h = _w;
        if (_w * _h < _minArea)
            _h++;
    }

    // Constructor for FIXED modules (Dimensions and Position known)
    Block(string &name, size_t w, size_t h, size_t x, size_t y)
        : Terminal(name, x, y), _w(w), _h(h), _minArea(w * h), _isFixed(true), _isGhost(false)
    {
        _x2 = _x1 + _w;
        _y2 = _y1 + _h;
    }

    ~Block() {}

    // basic access methods
    size_t getWidth(bool rotate = false) const { return rotate ? _h : _w; }
    size_t getHeight(bool rotate = false) const { return rotate ? _w : _h; }
    size_t getArea() const { return _h * _w; }
    size_t getMinArea() const { return _minArea; }
    bool isFixed() const { return _isFixed; }

    // NEW: Ghost getter
    bool isGhost() const { return _isGhost; }

    // set functions
    void setWidth(size_t w) { _w = w; }
    void setHeight(size_t h) { _h = h; }
    void setID(size_t id) { _id = id; }

    // Resize function for Soft Modules (Aspect Ratio = H / W)
    void resize(double aspectRatio)
    {
        if (_isFixed)
            return;

        // Safety check for ghosts with very small area
        if (_minArea == 0)
            return;

        _w = static_cast<size_t>(std::sqrt(_minArea / aspectRatio));
        if (_w == 0)
            _w = 1;
        _h = static_cast<size_t>(std::ceil((double)_minArea / _w));

        if (_w * _h < _minArea)
            _h++;
    }

    void setNode(Node *node) { _node = node; }
    Node *getNode() { return _node; }

private:
    size_t _w;
    size_t _h;
    size_t _minArea;
    bool _isFixed;
    bool _isGhost; // NEW: Ghost flag
    size_t _id;
    Node *_node;
};

class Net
{
public:
    Net() {}
    ~Net() {}

    const vector<Terminal *> getTermList() { return _termList; }
    size_t getDegree() { return _netDegree; }

    void addTerm(Terminal *term) { _termList.push_back(term); }
    void setDegree(size_t degree) { _netDegree = degree; }

    // Updated HPWL Calculation (Center-to-Center)
    // The problem statement specifies center-to-center distance for Manhattan distance
    double calcHPWL() const
    {
        if (_termList.empty())
            return 0.0;

        size_t minX = std::numeric_limits<size_t>::max();
        size_t maxX = 0;
        size_t minY = std::numeric_limits<size_t>::max();
        size_t maxY = 0;

        for (Terminal *term : _termList)
        {
            size_t cx = term->getCenterX();
            size_t cy = term->getCenterY();

            minX = std::min(minX, cx);
            maxX = std::max(maxX, cx);
            minY = std::min(minY, cy);
            maxY = std::max(maxY, cy);
        }

        return static_cast<double>((maxX - minX) + (maxY - minY));
    }

private:
    vector<Terminal *> _termList;
    size_t _netDegree;
};

#endif // MODULE_H