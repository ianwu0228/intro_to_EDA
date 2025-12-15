#ifndef NODE_H
#define NODE_H

using namespace std;

class Node
{
public:
    // Constructor
    Node(int id)
        : _id(id), _rotated(false),
          _parent(nullptr), _left(nullptr), _right(nullptr) {}

    // Getters
    int   getBlockIndex() const    { return _id; }
    bool  isRotated()     const    { return _rotated;    }
    Node* getParent()     const    { return _parent;     }
    Node* getLeft()       const    { return _left;       }
    Node* getRight()      const    { return _right;      }

    // Setters
    void setBlockIndex(int idx) { _id = idx;  }
    void setRotated(bool r)     { _rotated = r;       }
    void setParent(Node* p)     { _parent = p;        }
    void setLeft(Node* l)       { _left = l;          }
    void setRight(Node* r)      { _right = r;         }

private:
    int   _id;  // Index referring to a block in the Floorplanner's block array
    bool  _rotated;     // True if this node’s block is rotated
    Node* _parent;      // Pointer to the parent node
    Node* _left;        // Pointer to the left child
    Node* _right;       // Pointer to the right child
};

#endif // NODE_H
