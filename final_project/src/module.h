#ifndef MODULE_H
#define MODULE_H

#include <vector>
#include <string>
#include "node.h"  // Node
#include <ctime>     // for std::time
#include <cmath>     // for std::abs
#include <limits>

using namespace std;

class Terminal
{
public:
    // constructor and destructor
    Terminal(string& name, size_t x, size_t y) :
        _name(name), _x1(x), _y1(y), _x2(x), _y2(y) { }
    ~Terminal()  { }

    // basic access methods
    const string getName()  { return _name; }
    const size_t getX1()    { return _x1; }
    const size_t getX2()    { return _x2; }
    const size_t getY1()    { return _y1; }
    const size_t getY2()    { return _y2; }

    // set functions
    void setName(string& name) { _name = name; }
    void setPos(size_t x1, size_t y1, size_t x2, size_t y2) {
        _x1 = x1;   _y1 = y1;
        _x2 = x2;   _y2 = y2;
    }
    void setID(size_t id) { _id = id; }

protected:
    string      _name;      // module name
    size_t      _x1;        // min x coordinate of the terminal
    size_t      _y1;        // min y coordinate of the terminal
    size_t      _x2;        // max x coordinate of the terminal
    size_t      _y2;        // max y coordinate of the terminal
    size_t      _id;
};


class Block : public Terminal
{
public:
    // constructor and destructor
    Block(string& name, size_t w, size_t h) :
        Terminal(name, 0, 0), _w(w), _h(h) { 
        }
    ~Block() { }

    // basic access methods
    const size_t getWidth(bool rotate = false)  { return rotate? _h: _w; }
    const size_t getHeight(bool rotate = false) { return rotate? _w: _h; }
    const size_t getArea()  { return _h * _w; }
    static size_t getMaxX() { return _maxX; }
    static size_t getMaxY() { return _maxY; }

    // set functions
    void setWidth(size_t w)         { _w = w; }
    void setHeight(size_t h)        { _h = h; }
    static void setMaxX(size_t x)   { _maxX = x; }
    static void setMaxY(size_t y)   { _maxY = y; }
    void setID(size_t id)           { _id = id; }


    // other member functions
    void setNode(Node* node) { _node = node; }
    Node* getNode() { return _node; }


private:
    size_t          _w;         // width of the block
    size_t          _h;         // height of the block
    static size_t   _maxX;      // maximum x coordinate for all blocks
    static size_t   _maxY;      // maximum y coordinate for all blocks
    size_t          _id;        // id of the block
    Node*           _node;   // pointer to the parent node
};


class Net
{
public:
    // constructor and destructor
    Net() { }
    ~Net()  { }

    // basic access methods
    const vector<Terminal*> getTermList()   { return _termList; }
    size_t getDegree() { return _netDegree; }

    // modify methods
    void addTerm(Terminal* term) { _termList.push_back(term); }
    void setDegree(size_t degree) { _netDegree = degree; }

    
    // other member functions
    // double calcHPWL() const {
    //     if (_termList.empty()) return 0.0;
    
    //     size_t minX = std::numeric_limits<size_t>::max();
    //     size_t maxX = 0;
    //     size_t minY = std::numeric_limits<size_t>::max();
    //     size_t maxY = 0;
    
    //     for (Terminal* term : _termList) {
    //         minX = std::min(minX, term->getX1());
    //         maxX = std::max(maxX, term->getX2());  // assuming X2 is exclusive
    //         minY = std::min(minY, term->getY1());
    //         maxY = std::max(maxY, term->getY2());  // assuming Y2 is exclusive
    //     }
    
    //     return static_cast<double>((maxX - minX) + (maxY - minY));
    // }
    double calcHPWL() const {
        if (_termList.empty()) return 0.0;
    
        size_t minX = std::numeric_limits<size_t>::max();
        size_t maxX = 0;
        size_t minY = std::numeric_limits<size_t>::max();
        size_t maxY = 0;
    
        for (Terminal* term : _termList) {
            // Check both X1 and X2 for each terminal
            minX = std::min(minX, std::min(term->getX1(), term->getX2()));
            maxX = std::max(maxX, std::max(term->getX1(), term->getX2()));
            
            // Check both Y1 and Y2 for each terminal
            minY = std::min(minY, std::min(term->getY1(), term->getY2()));
            maxY = std::max(maxY, std::max(term->getY1(), term->getY2()));
        }
    
        return static_cast<double>((maxX - minX) + (maxY - minY));
    }
    

private:
    vector<Terminal*>   _termList;  // list of terminals the net is connected to
    size_t _netDegree; // degree of the net
};

#endif  // MODULE_H
