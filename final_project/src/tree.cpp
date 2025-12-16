#include "tree.h"
#include <iostream>
#include <cstdlib> // for rand()
#include <vector>
#include <algorithm> // for std::random_shuffle
#include <ctime>     // for std::time
#include <cmath>     // for std::abs
#include <fstream>

using namespace std;

// void Tree::buildInitial()
// {
//     if (_blocks.empty()) return;

//     // Clear just in case
//     _nodes.clear();
//     _nodes.reserve(_blocks.size());

//     // Create one node per block
//     for (size_t i = 0; i < _blocks.size(); ++i) {
//         _nodes.emplace_back(i);                          // Node with block ID
//         _blocks[i].setNode(&_nodes.back());              // Point Block -> Node
//     }

//     // Build left-linked chain: node[i] is the left child of node[i - 1]
//     _root = &_nodes[0];          // Set first node as root
//     _root->setParent(nullptr);

//     for (size_t i = 1; i < _nodes.size()/2; ++i) {
//         Node* parent = &_nodes[i - 1];
//         Node* child  = &_nodes[i];

//         parent->setLeft(child);      // B*-tree: left = rightward placement
//         child->setParent(parent);
//     }
//     for (size_t i = _nodes.size()/2; i < _nodes.size(); ++i) {
//         Node* parent = &_nodes[i - 1];
//         Node* child  = &_nodes[i];

//         parent->setRight(child);      // B*-tree: left = rightward placement
//         child->setParent(parent);
//     }
// }

// tree.cpp

void Tree::resizeRandom()
{
    if (_blocks.empty())
        return;

    // Pick a random block
    int randIdx = rand() % _blocks.size();

    // We must ensure we don't try to resize a Fixed block (though _blocks should only contain soft ones now)
    // But double check just in case:
    if (_blocks[randIdx].isFixed())
        return;

    // Generate random Aspect Ratio between 0.5 and 2.0
    // (rand / RAND_MAX) gives 0.0 to 1.0
    // formula: 0.5 + (0..1.5)
    double ar = 0.5 + (static_cast<double>(rand()) / RAND_MAX) * 1.5;

    _blocks[randIdx].resize(ar);
}

void Tree::buildInitial()
{
    if (_blocks.empty())
        return;

    // Clear just in case
    _nodes.clear();
    _nodes.reserve(_blocks.size());

    // Create one node per block
    for (size_t i = 0; i < _blocks.size(); ++i)
    {
        _nodes.emplace_back(i);             // Node with block ID
        _blocks[i].setNode(&_nodes.back()); // Point Block -> Node
    }

    // Build complete binary tree
    _root = &_nodes[0]; // Set first node as root
    _root->setParent(nullptr);

    for (size_t i = 0; i < _nodes.size(); ++i)
    {
        // Calculate indices of left and right children
        size_t leftIdx = 2 * i + 1;
        size_t rightIdx = 2 * i + 2;

        // Set left child if exists
        if (leftIdx < _nodes.size())
        {
            _nodes[i].setLeft(&_nodes[leftIdx]);
            _nodes[leftIdx].setParent(&_nodes[i]);
        }

        // Set right child if exists
        if (rightIdx < _nodes.size())
        {
            _nodes[i].setRight(&_nodes[rightIdx]);
            _nodes[rightIdx].setParent(&_nodes[i]);
        }
    }
}

// void Tree::buildInitial() {
//     if (_blocks.empty()) return;

//     _nodes.clear();
//     _nodes.reserve(_blocks.size());

//     // 1. Create nodes
//     for (size_t i = 0; i < _blocks.size(); ++i) {
//         _nodes.emplace_back(i);
//         _blocks[i].setNode(&_nodes.back());
//     }

//     // 2. Build balanced tree recursively
//     _root = buildBalancedRecursive(0, _nodes.size() - 1);
// }

// Node* Tree::buildBalancedRecursive(int l, int r) {
//     if (l > r) return nullptr;

//     int mid = (l + r) / 2;
//     Node* root = &_nodes[mid];

//     Node* leftChild = buildBalancedRecursive(l, mid - 1);
//     Node* rightChild = buildBalancedRecursive(mid + 1, r);

//     root->setLeft(leftChild);
//     root->setRight(rightChild);

//     if (leftChild) leftChild->setParent(root);
//     if (rightChild) rightChild->setParent(root);

//     return root;
// }

void Tree::rotateRandom()
{
    // Randomly rotate a node in the tree
    if (_nodes.empty())
        return;
    // Select a random node index
    size_t randomIndex = rand() % _nodes.size();
    Node *randomNode = &_nodes[randomIndex];

    // Toggle rotation state
    randomNode->setRotated(!randomNode->isRotated());
}

void Tree::deleteNode(Node *u)
{
    if (!u)
        return;

    Node *parent = u->getParent();
    Node *left = u->getLeft();
    Node *right = u->getRight();

    // Case 1: no children
    if (!left && !right)
    {
        if (parent)
        {
            if (parent->getLeft() == u)
                parent->setLeft(nullptr);
            else if (parent->getRight() == u)
                parent->setRight(nullptr);
        }
        else
        {
            _root = nullptr; // u is root
            cout << "error: root is deleted with no children" << endl;
        }
        u->setLeft(nullptr);
        u->setRight(nullptr);
        u->setParent(nullptr); // Disconnect from parent
        return;
    }

    // Case 2: only one child
    Node *child = left ? left : right;
    if (!left || !right)
    {
        if (parent)
        {
            if (parent->getLeft() == u)
                parent->setLeft(child);
            else
                parent->setRight(child);
        }
        else
        {
            _root = child;
        }
        child->setParent(parent);
        u->setLeft(nullptr);
        u->setRight(nullptr);
        u->setParent(nullptr); // Disconnect from parent
        return;
    }

    // Case 3: two children
    // Promote left, and attach right at rightmost of left's right spine
    Node *newChild = left;

    // Attach right subtree to rightmost node of left subtree
    Node *rightmost = left;
    while (rightmost->getRight())
    {
        rightmost = rightmost->getRight();
    }
    rightmost->setRight(right);
    right->setParent(rightmost);

    // Reconnect to parent
    if (parent)
    {
        if (parent->getLeft() == u)
            parent->setLeft(newChild);
        else
            parent->setRight(newChild);
    }
    else
    {
        _root = newChild;
    }
    newChild->setParent(parent);
    u->setLeft(nullptr);
    u->setRight(nullptr);
    u->setParent(nullptr); // Disconnect from parent
}

// void Tree::deleteNode(Node* u)
// {
//     if (!u) return;

//     Node* parent = u->getParent();
//     Node* left   = u->getLeft();
//     Node* right  = u->getRight();

//     // Case 1: no children
//     if (!left && !right) {
//         if (parent) {
//             if (parent->getLeft() == u) parent->setLeft(nullptr);
//             else if (parent->getRight() == u) parent->setRight(nullptr);
//         } else {
//             _root = nullptr; // u was root
//         }
//     }

//     // Case 2: one child
//     else if (!left || !right) {
//         Node* child = left ? left : right;
//         if (parent) {
//             if (parent->getLeft() == u) parent->setLeft(child);
//             else parent->setRight(child);
//         } else {
//             _root = child;  // u was root
//         }
//         child->setParent(parent);
//     }

//     // Case 3: two children
//     else {
//         // Promote left
//         Node* newSubtree = left;

//         // Attach right subtree to the rightmost node of left
//         Node* rightmost = left;
//         while (rightmost->getRight()) {
//             rightmost = rightmost->getRight();
//         }
//         rightmost->setRight(right);
//         right->setParent(rightmost);

//         if (parent) {
//             if (parent->getLeft() == u) parent->setLeft(newSubtree);
//             else parent->setRight(newSubtree);
//         } else {
//             _root = newSubtree;
//         }
//         newSubtree->setParent(parent);
//     }

//     // Clear u
//     u->setLeft(nullptr);
//     u->setRight(nullptr);
//     u->setParent(nullptr);
// }

void Tree::insertNode(Node *u, Node *target, bool asLeftChild)
{
    if (!u || !target)
    {
        cout << "error: null pointer in insertNode" << endl;
        return;
    }

    // Set parent of u to target
    u->setParent(target);

    // Set left/right child of target to u
    if (asLeftChild)
    {
        target->setLeft(u);
    }
    else
    {
        target->setRight(u);
    }
}

void Tree::deleteAndInsert()
{
    if (_nodes.size() <= 1)
        return;

    // Step 1: select a random node to delete (not root is safer)
    Node *u;
    // do {
    //     u = &_nodes[rand() % _nodes.size()];
    // } while (u == _root);  // Avoid deleting root for now
    u = &_nodes[rand() % _nodes.size()];

    deleteNode(u);

    // Step 2: build a list of valid insertion targets
    vector<Node *> candidates;
    for (Node &n : _nodes)
    {
        if (&n == u)
            continue;
        if (n.getLeft() == nullptr || n.getRight() == nullptr)
            candidates.push_back(&n);
    }

    if (candidates.empty())
        return; // No valid place to reinsert

    // Step 3: pick one and insert u as child
    Node *target = candidates[rand() % candidates.size()];
    bool asLeft;
    if (target->getLeft() == nullptr && target->getRight() == nullptr)
    {
        // both available — pick randomly
        asLeft = rand() % 2;
        // asLeft = true;
    }
    else if (target->getLeft() == nullptr)
    {
        asLeft = true;
    }
    else if (target->getRight() == nullptr)
    {
        asLeft = false;
    }
    else
    {
        // no slot available — skip or pick a new target
        return;
    }
    insertNode(u, target, asLeft);
}

// void Tree::deleteAndInsert()
// {
//     if (_nodes.size() <= 1) return;

//     Node* u = nullptr;
//     // Pick node to delete (allow root now)
//     u = &_nodes[rand() % _nodes.size()];

//     deleteNode(u);  // May update _root

//     // Gather valid insert positions
//     vector<Node*> candidates;
//     for (Node& n : _nodes) {
//         if (&n == u) continue;
//         if (n.getLeft() == nullptr || n.getRight() == nullptr)
//             candidates.push_back(&n);
//     }

//     if (candidates.empty()) return;

//     Node* target = candidates[rand() % candidates.size()];

//     bool asLeft;
//     if (target->getLeft() == nullptr && target->getRight() == nullptr) {
//         asLeft = rand() % 2;
//     } else if (target->getLeft() == nullptr) {
//         asLeft = true;
//     } else if (target->getRight() == nullptr) {
//         asLeft = false;
//     } else {
//         return;
//     }

//     insertNode(u, target, asLeft);
// }

// void Tree::swapRandomNodes()
// {
//     if (_nodes.size() <= 1) return;

//     Node* u;
//     Node* v;

//     do {
//         u = &_nodes[rand() % _nodes.size()];
//         v = &_nodes[rand() % _nodes.size()];
//     } while (u == v);  // allow root swap if you handle _root properly

//     ////////////// conditions //////////////
//     // make sure u and v are not direct parent-child
//     if (u == v->getParent() || v == u->getParent()) return;
//     // make sure they don't share the same parent
//     if ((u->getParent() == v->getParent())) return;
//     // make sure they are not in the same subtree
//     if (isDescendant(u, v) || isDescendant(v, u)) return;
//     //////////////// end conditions //////////////

//     // Save neighbors
//     Node* pu = u->getParent();
//     Node* pl = u->getLeft();
//     Node* pr = u->getRight();

//     Node* qv = v->getParent();
//     Node* ql = v->getLeft();
//     Node* qr = v->getRight();

//     // Swap parents
//     u->setParent(qv);
//     v->setParent(pu);

//     // Swap children
//     u->setLeft(ql);
//     u->setRight(qr);
//     v->setLeft(pl);
//     v->setRight(pr);

//     // Fix parents’ child pointers
//     if (qv) {
//         if (qv->getLeft() == v) qv->setLeft(u);
//         else if (qv->getRight() == v) qv->setRight(u);
//     } else {
//         _root = u;  // v was root
//     }

//     if (pu) {
//         if (pu->getLeft() == u) pu->setLeft(v);
//         else if (pu->getRight() == u) pu->setRight(v);
//     } else {
//         _root = v;  // u was root
//     }

//     // Fix children’s parent pointers
//     if (ql) ql->setParent(u);
//     if (qr) qr->setParent(u);
//     if (pl) pl->setParent(v);
//     if (pr) pr->setParent(v);

// }

void Tree::swapRandomNodes()
{
    if (_nodes.size() <= 1)
        return;

    Node *u;
    Node *v;

    do
    {
        u = &_nodes[rand() % _nodes.size()];
        v = &_nodes[rand() % _nodes.size()];
    } while (u == v); // allow root swap if you handle _root properly

    //////////////// handle special cases

    // if u and v are parent-child
    //  make sure u and v are not direct parent-child
    if (u == v->getParent() || v == u->getParent())
    {
        if (u == v->getParent())
        {
            // Case: u is parent of v → promote v, demote u

            Node *grandparent = u->getParent();
            Node *ul = u->getLeft();
            Node *ur = u->getRight();
            Node *vl = v->getLeft();
            Node *vr = v->getRight();
            bool vIsLeft = (u->getLeft() == v);

            // 1. Connect grandparent to v
            if (grandparent)
            {
                if (grandparent->getLeft() == u)
                    grandparent->setLeft(v);
                else
                    grandparent->setRight(v);
            }
            else
            {
                _root = v;
            }

            v->setParent(grandparent);
            u->setParent(v);

            // 2. Reassign child links
            if (vIsLeft)
            {
                v->setLeft(u);
                v->setRight(ur == v ? nullptr : ur);
                if (v->getRight())
                    v->getRight()->setParent(v);
            }
            else
            {
                v->setRight(u);
                v->setLeft(ul == v ? nullptr : ul);
                if (v->getLeft())
                    v->getLeft()->setParent(v);
            }

            // u adopts v’s previous children
            u->setLeft(vl == u ? nullptr : vl);
            u->setRight(vr == u ? nullptr : vr);
            if (u->getLeft())
                u->getLeft()->setParent(u);
            if (u->getRight())
                u->getRight()->setParent(u);

            return;
        }
        if (v == u->getParent())
        {
            // Just flip roles and reuse logic
            std::swap(u, v);
            // Now u is the parent and v is the child
            // Reuse same code block
            // Note: DON'T call swapRandomNodes() recursively; re-enter the block directly
            Node *grandparent = u->getParent();
            Node *ul = u->getLeft();
            Node *ur = u->getRight();
            Node *vl = v->getLeft();
            Node *vr = v->getRight();
            bool vIsLeft = (u->getLeft() == v);

            if (grandparent)
            {
                if (grandparent->getLeft() == u)
                    grandparent->setLeft(v);
                else
                    grandparent->setRight(v);
            }
            else
            {
                _root = v;
            }

            v->setParent(grandparent);
            u->setParent(v);

            if (vIsLeft)
            {
                v->setLeft(u);
                v->setRight(ur == v ? nullptr : ur);
                if (v->getRight())
                    v->getRight()->setParent(v);
            }
            else
            {
                v->setRight(u);
                v->setLeft(ul == v ? nullptr : ul);
                if (v->getLeft())
                    v->getLeft()->setParent(v);
            }

            u->setLeft(vl == u ? nullptr : vl);
            u->setRight(vr == u ? nullptr : vr);
            if (u->getLeft())
                u->getLeft()->setParent(u);
            if (u->getRight())
                u->getRight()->setParent(u);

            return;
        }
    }
    /////////////////// end conditions //////////////

    // if share the same parent, swap them in place
    // if ((u->getParent() == v->getParent())) return;
    if (u->getParent() == v->getParent())
    {
        Node *p = u->getParent();
        Node *ul = u->getLeft();
        Node *ur = u->getRight();
        Node *vl = v->getLeft();
        Node *vr = v->getRight();

        // Swap child pointers in parent
        if (p->getLeft() == u)
        {
            p->setLeft(v);
            p->setRight(u);
        }
        else
        {
            p->setLeft(u);
            p->setRight(v);
        }

        // Swap their children
        u->setLeft(vl);
        u->setRight(vr);
        v->setLeft(ul);
        v->setRight(ur);

        // Update parent of children
        if (u->getLeft())
            u->getLeft()->setParent(u);
        if (u->getRight())
            u->getRight()->setParent(u);
        if (v->getLeft())
            v->getLeft()->setParent(v);
        if (v->getRight())
            v->getRight()->setParent(v);

        // Keep parent unchanged
        u->setParent(p);
        v->setParent(p);

        return;
    }
    ///////////////// end conditions //////////////

    // in the same subtree
    // if (isDescendant(u, v) || isDescendant(v, u)) return;
    if (isDescendant(u, v) || isDescendant(v, u))
    {
        // Make sure u is the ancestor
        if (isDescendant(v, u))
            std::swap(u, v);

        Node *pu = u->getParent();
        Node *pv = v->getParent();

        Node *ul = u->getLeft();
        Node *ur = u->getRight();
        Node *vl = v->getLeft();
        Node *vr = v->getRight();

        bool uIsLeftChild = (pu && pu->getLeft() == u);
        bool vIsLeftChild = (pv && pv->getLeft() == v);

        // Step 1: Detach v from its parent
        if (pv)
        {
            if (vIsLeftChild)
                pv->setLeft(nullptr);
            else
                pv->setRight(nullptr);
        }

        // Step 2: Detach u from its parent
        if (pu)
        {
            if (uIsLeftChild)
                pu->setLeft(v);
            else
                pu->setRight(v);
        }
        else
        {
            _root = v; // v becomes new root
        }

        // Step 3: v takes u’s children (except v itself)
        if (ul != v)
            v->setLeft(ul);
        else
            v->setLeft(nullptr);
        if (ur != v)
            v->setRight(ur);
        else
            v->setRight(nullptr);
        if (v->getLeft())
            v->getLeft()->setParent(v);
        if (v->getRight())
            v->getRight()->setParent(v);
        v->setParent(pu);

        // Step 4: u moves to v’s position
        u->setParent(pv);
        if (vl != u)
            u->setLeft(vl);
        else
            u->setLeft(nullptr);
        if (vr != u)
            u->setRight(vr);
        else
            u->setRight(nullptr);
        if (u->getLeft())
            u->getLeft()->setParent(u);
        if (u->getRight())
            u->getRight()->setParent(u);

        if (pv)
        {
            if (vIsLeftChild)
                pv->setLeft(u);
            else
                pv->setRight(u);
        }

        return;
    }
    //////////////// end conditions //////////////

    ///////////// end of special cases /////////////

    //////////////// general case /////////////////
    // Save neighbors
    Node *pu = u->getParent();
    Node *pl = u->getLeft();
    Node *pr = u->getRight();

    Node *qv = v->getParent();
    Node *ql = v->getLeft();
    Node *qr = v->getRight();

    // Swap parents
    u->setParent(qv);
    v->setParent(pu);

    // Swap children
    u->setLeft(ql);
    u->setRight(qr);
    v->setLeft(pl);
    v->setRight(pr);

    // Fix parents’ child pointers
    if (qv)
    {
        if (qv->getLeft() == v)
            qv->setLeft(u);
        else if (qv->getRight() == v)
            qv->setRight(u);
    }
    else
    {
        _root = u; // v was root
    }

    if (pu)
    {
        if (pu->getLeft() == u)
            pu->setLeft(v);
        else if (pu->getRight() == u)
            pu->setRight(v);
    }
    else
    {
        _root = v; // u was root
    }

    // Fix children’s parent pointers
    if (ql)
        ql->setParent(u);
    if (qr)
        qr->setParent(u);
    if (pl)
        pl->setParent(v);
    if (pr)
        pr->setParent(v);
}

bool Tree::isDescendant(Node *ancestor, Node *candidate)
{
    if (!ancestor)
        return false;
    if (ancestor == candidate)
        return true;
    return isDescendant(ancestor->getLeft(), candidate) ||
           isDescendant(ancestor->getRight(), candidate);
}

void Tree::clearContour()
{
    ContourSegment *cur = _contourHead;
    while (cur)
    {
        ContourSegment *next = cur->next;
        delete cur;
        cur = next;
    }
    _contourHead = nullptr;
}

// void Tree::pack()
// {
//     clearContour();
//     _contourHead = new ContourSegment(0, std::numeric_limits<size_t>::max(), 0);
//     pack(_root, 0); // Start at origin
// }

// File: src/tree.cpp

void Tree::pack()
{
    clearContour();
    // flat ground contour
    _contourHead = new ContourSegment(0, std::numeric_limits<size_t>::max(), 0);
    pack(_root, 0);
}

void Tree::pack(Node *node, size_t baseX)
{
    if (!node)
        return;

    Block &blk = _blocks[node->getBlockIndex()];
    bool rotated = node->isRotated();
    size_t width = blk.getWidth(rotated);
    size_t height = blk.getHeight(rotated);

    size_t baseY = findMaxY(baseX, baseX + width);

    blk.setPos(baseX, baseY, baseX + width, baseY + height);

    updateContour(baseX, baseX + width, baseY + height);

    if (node->getLeft())
    {
        pack(node->getLeft(), baseX + width);
    }

    if (node->getRight())
    {
        pack(node->getRight(), baseX);
    }
}

double Tree::findMaxY(size_t x1, size_t x2) const
{
    size_t maxY = 0;

    ContourSegment *cur = _contourHead;

    // Traverse until we find the first segment that overlaps with x1
    while (cur && cur->x_end <= x1)
    {
        cur = cur->next;
    }

    // Now process segments overlapping with [x1, x2)
    while (cur && cur->x_start < x2)
    {
        maxY = std::max(maxY, cur->height);
        cur = cur->next;
    }

    return maxY;
}

void Tree::updateContour(size_t x1, size_t x2, size_t newHeight)
{
    ContourSegment *cur = _contourHead;

    // Step 1: Traverse to first segment that may overlap with x1
    while (cur && cur->x_end <= x1)
    {
        cur = cur->next;
    }

    // Step 2: Split start if needed
    if (cur && cur->x_start < x1 && cur->x_end > x1)
    {
        // Split into [cur->x_start, x1) and [x1, cur->x_end)
        ContourSegment *left = new ContourSegment(cur->x_start, x1, cur->height);
        ContourSegment *right = new ContourSegment(x1, cur->x_end, cur->height);

        // Re-link
        left->prev = cur->prev;
        left->next = right;
        right->prev = left;
        right->next = cur->next;

        if (left->prev)
            left->prev->next = left;
        else
            _contourHead = left;

        if (right->next)
            right->next->prev = right;

        delete cur;
        cur = right;
    }

    // Step 3: Start deleting all segments fully within [x1, x2)
    while (cur && cur->x_start < x2)
    {
        ContourSegment *next = cur->next;

        // If this segment extends beyond x2, split it
        if (cur->x_end > x2)
        {
            ContourSegment *tail = new ContourSegment(x2, cur->x_end, cur->height);
            tail->next = cur->next;
            tail->prev = cur->prev;

            if (tail->next)
                tail->next->prev = tail;
            if (tail->prev)
                tail->prev->next = tail;
            else
                _contourHead = tail;

            delete cur;
            cur = nullptr;
            break;
        }

        // Remove cur from the list
        if (cur->prev)
            cur->prev->next = cur->next;
        else
            _contourHead = cur->next;

        if (cur->next)
            cur->next->prev = cur->prev;

        delete cur;
        cur = next;
    }

    // Step 4: Insert the new segment [x1, x2) with newHeight
    // Find insert position: after x1's previous segment
    ContourSegment *insertAfter = _contourHead;
    ContourSegment *prevNode = nullptr;
    while (insertAfter && insertAfter->x_start < x1)
    {
        prevNode = insertAfter;
        insertAfter = insertAfter->next;
    }

    ContourSegment *newSeg = new ContourSegment(x1, x2, newHeight);
    newSeg->prev = prevNode;
    newSeg->next = insertAfter;

    if (prevNode)
        prevNode->next = newSeg;
    else
        _contourHead = newSeg;

    if (insertAfter)
        insertAfter->prev = newSeg;
}

void Tree::checkContour() const
{
    ContourSegment *cur = _contourHead;
    cout << "==========================" << endl;
    cout << "Checking contour..." << endl;
    while (cur)
    {
        if (cur->x_start >= cur->x_end)
        {
            cout << "Invalid segment: [" << cur->x_start << ", " << cur->x_end << ")" << endl;
        }
        if (cur->next && cur->next->prev != cur)
        {
            cout << "Broken next-prev link at: [" << cur->x_start << ", " << cur->x_end << ")" << endl;
        }
        if (cur->prev && cur->prev->next != cur)
        {
            cout << "Broken prev-next link at: [" << cur->x_start << ", " << cur->x_end << ")" << endl;
        }
        cur = cur->next;
    }
    cout << "Contour check complete." << endl;
    cout << "==========================" << endl;
}

void Tree::printContour() const
{
    cout << "[Contour] ";
    size_t segCount = 0;
    ContourSegment *cur = _contourHead;
    while (cur)
    {
        segCount++;
        cout << "[" << cur->x_start << ", " << cur->x_end << ") @ height "
             << cur->height << "  ";
        cur = cur->next;
        cout << endl;
    }
    cout << "NULL" << endl;
    cout << "Total segments: " << segCount << endl;
}

// void Tree::printTree(Node* node, int depth) const {
//     if (!node) return;

//     string indent(depth * 2, ' ');  // 2 spaces per level
//     int id = node->getBlockIndex();
//     bool rotated = node->isRotated();
//     cout << indent << "Block " << id
//          << (rotated ? " (rotated)" : "")
//          << endl;

//     // Traverse children
//     printTree(node->getLeft(), depth + 1);   // left = to the right
//     printTree(node->getRight(), depth + 1);  // right = above
// }

void Tree::printTree(Node *node, int depth, int &count) const
{
    if (!node)
        return;
    ++count;

    string indent(depth * 2, ' ');
    // cout << indent << "Block " << node->getBlockIndex() << endl;
    cout << indent << "Block " << node->getBlockIndex() << endl;

    printTree(node->getLeft(), depth + 1, count);
    printTree(node->getRight(), depth + 1, count);
}

// void Tree::exportToFile(const string& filename) const {
//     ofstream fout(filename);
//     if (!fout.is_open()) {
//         cerr << "Cannot open output file: " << filename << endl;
//         return;
//     }

//     for (const Node& node : _nodes) {
//         const Block& blk = _blocks[node.getBlockIndex()];
//         fout << _blocks[node.getBlockIndex()].getName() << " "
//              << _blocks[node.getBlockIndex()].getX1() << " " << _blocks[node.getBlockIndex()].getY1() << " "
//              << _blocks[node.getBlockIndex()].getX2() << " " << _blocks[node.getBlockIndex()].getY2() << " "
//              << (node.isRotated() ? "rotated" : "normal") << endl;
//     }

//     fout.close();
// }

void Tree::exportToFile(fstream &output, size_t outlineW, size_t outlineH, const std::chrono::milliseconds &duration, size_t outputCost, size_t outputWL, size_t outputArea, size_t maxX, size_t maxY) const
{
    // std::ofstream fout(filename);
    if (!output.is_open())
    {
        std::cerr << "Cannot open output file" << std::endl;
        return;
    }

    // output << "OUTLINE " << outlineW << " " << outlineH << "\n";

    // cost
    output << outputCost << "\n";
    // wirelength
    output << outputWL << "\n";
    // area
    output << outputArea << "\n";
    // width and height
    output << maxX << " " << maxY << "\n";
    // duration
    output << duration.count() * 0.001 << "\n";

    for (const Node &node : _nodes)
    {
        Block &blk = _blocks[node.getBlockIndex()];
        output << blk.getName() << " "
               << blk.getX1() << " " << blk.getY1() << " "
               << blk.getX2() << " " << blk.getY2() << "\n";
        //  << (node.isRotated() ? "rotated" : "normal") << "\n";
    }

    output.close();
}

void Tree::exportToFile_visualize(fstream &output, size_t outlineW, size_t outlineH, const std::chrono::milliseconds &duration, size_t outputCost, size_t outputWL, size_t outputArea, size_t maxX, size_t maxY) const
{
    // std::ofstream fout(filename);
    if (!output.is_open())
    {
        std::cerr << "Cannot open output file" << std::endl;
        return;
    }

    output << "OUTLINE " << outlineW << " " << outlineH << "\n";

    // cost
    output << outputCost << "\n";
    // wirelength
    output << outputWL << "\n";
    // area
    output << outputArea << "\n";
    // width and height
    output << maxX << " " << maxY << "\n";
    // duration
    output << duration.count() * 0.001 << "\n";

    for (const Node &node : _nodes)
    {
        Block &blk = _blocks[node.getBlockIndex()];
        output << blk.getName() << " "
               << blk.getX1() << " " << blk.getY1() << " "
               << blk.getX2() << " " << blk.getY2() << "\n";
        //  << (node.isRotated() ? "rotated" : "normal") << "\n";
    }

    output.close();
}