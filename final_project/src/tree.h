#ifndef TREE_H
#define TREE_H

#include <vector>
#include <cstdlib> // for rand()
#include "node.h"
#include "module.h" // Block, Terminal, Net
#include <chrono>

using namespace std;

class Tree
{
public:
    Tree(vector<Block> &blocks)
        : _blocks(blocks), _root(nullptr), _contourHead(nullptr)
    {
        // _nodes.reserve(blocks.size()); // Reserve space for nodes
        // for (size_t i = 0; i < blocks.size(); ++i) {
        //     _nodes.emplace_back(i); // equals to _nodes.push_back(Node(i))
        // }
    }

    void buildInitial();                                // trivial B*-tree construction (e.g. linear left-chain)
    void rotateRandom();                                // perturbation 1
    void deleteAndInsert();                             // perturbation 2
    void swapRandomNodes();                             // perturbation 3
    void resizeRandom();                                // perturbation 4 for soft modules
    bool isDescendant(Node *ancestor, Node *candidate); // check if candidate is a descendant of ancestor
    Node *buildBalancedRecursive(int l, int r);         // build a balanced tree recursively

    // packing related functions
    void pack();                                             // compute (x1, y1, x2, y2) for each block
    double findMaxY(size_t x1, size_t x2) const;             // find max y in contour between x1 and x2
    void updateContour(size_t x1, size_t x2, size_t height); // update contour after placing a block
    void clearContour();                                     // clear the contour list
    void printContour() const;                               // print the contour for debugging
    void checkContour() const;                               // check the contour for debugging

    Node *getRoot() const { return _root; }
    const vector<Node> &getNodes() const { return _nodes; }

    /// Print the B*-tree structure
    void printTree(Node *node, int depth, int &count) const;
    int getNumofNodes() const { return _nodes.size(); }

    // horizontal segment of the contour
    struct ContourSegment
    {
        size_t x_start, x_end; // horizontal range
        size_t height;         // top y of the segment
        ContourSegment *prev;
        ContourSegment *next;

        ContourSegment(size_t x1, size_t x2, size_t h)
            : x_start(x1), x_end(x2), height(h), prev(nullptr), next(nullptr) {}
    };

    // visualization function (for debugging)
    // void exportToFile(const string& filename) const;

    void exportToFile(fstream &output, size_t outlineW, size_t outlineH, const std::chrono::milliseconds &duration, size_t outputCost, size_t outputWL, size_t outputArea, size_t maxX, size_t maxY) const;
    void exportToFile_visualize(fstream &output, size_t outlineW, size_t outlineH, const std::chrono::milliseconds &duration, size_t outputCost, size_t outputWL, size_t outputArea, size_t maxX, size_t maxY) const;

    Tree &operator=(const Tree &other)
    {
        if (this != &other)
        {
            // Reuse the existing _blocks reference
            _nodes = other._nodes;
            _root = nullptr;

            // Reconnect pointers in _nodes
            for (size_t i = 0; i < _nodes.size(); ++i)
            {
                if (other._nodes[i].getParent())
                    _nodes[i].setParent(&_nodes[other._nodes[i].getParent()->getBlockIndex()]);
                if (other._nodes[i].getLeft())
                    _nodes[i].setLeft(&_nodes[other._nodes[i].getLeft()->getBlockIndex()]);
                if (other._nodes[i].getRight())
                    _nodes[i].setRight(&_nodes[other._nodes[i].getRight()->getBlockIndex()]);
            }

            // Restore root
            if (other._root)
                _root = &_nodes[other._root->getBlockIndex()];
        }
        return *this;
    }

private:
    vector<Node> _nodes;    // One node per block (1-1 mapping by index)
    Node *_root;            // root of the B*-tree
    vector<Block> &_blocks; // Reference to external block array

    void pack(Node *v, size_t baseX); // internal recursive function

    void deleteNode(Node *u);                                 // helper: remove a node from the tree
    void insertNode(Node *u, Node *target, bool asLeftChild); // helper: insert node into new location

    ContourSegment *_contourHead; // head of the contour list
};

#endif // TREE_H
