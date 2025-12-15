#include "cluster.h"

Cluster::Cluster(std::vector<Module*> M) : Module() {
    leaf = M;
    setPosition(Point(0, 0));
    setRotate(0);
}

Cluster::Cluster(std::vector<std::unique_ptr<Module>> &M) : Module() {
    leaf.reserve(M.size());
    for(auto &m : M) {
        leaf.push_back(m.get());
    }
    setPosition(Point(0, 0));
    setRotate(0);
}

void Cluster::setRotate(bool val ) {
    if(val != this->getRotate() )
        rotate();
}

void Cluster::setPosition(const Point &pos) {
    Point delta = pos - getPosition();
    for(auto m : leaf) {
        m->setPosition(m->getPosition() + delta);
    }
    Module::setPosition(pos);
}

void Cluster::collectAllLeaves(std::vector<Module*>& out) const {
    std::unordered_set<const void*> visited;
    collectAllLeaves(out, visited);
}

void Cluster::collectAllClusters(std::vector<Cluster*>& out, bool include_self) const {
    std::unordered_set<const void*> visited;
    collectAllClusters(out, include_self, visited);
}

void Cluster::collectAll(std::vector<Module*>& leaves,
                         std::vector<Cluster*>& clusters,
                         bool include_self) const {
    std::unordered_set<const void*> visitedL, visitedC;
    collectAllClusters(clusters, include_self, visitedC);
    collectAllLeaves(leaves, visitedL);
    int num_clusters = clusters.size();
    int num_leaves = leaves.size();
    printf("num_clusters: %d, num_leaves: %d\n", num_clusters, num_leaves);

}


void Cluster::collectAllLeaves(std::vector<Module*>& out,
                               std::unordered_set<const void*>& visited) const {
    if (!visited.insert(this).second) return;

    for (Module* m : leaf) {
        if (!m) continue;
        if (auto* c = dynamic_cast<Cluster*>(m)) {
            c->collectAllLeaves(out, visited);
        } else {
            if (visited.insert(m).second)
                out.push_back(m);
        }
    }
}

void Cluster::collectAllClusters(std::vector<Cluster*>& out, bool include_self,
                                 std::unordered_set<const void*>& visited) const {
    if (include_self) {
        if (visited.insert(this).second)
            out.push_back(const_cast<Cluster*>(this));
    } else {
        visited.insert(this);
    }

    for (Module* m : leaf) {
        if (!m) continue;
        if (auto* c = dynamic_cast<Cluster*>(m)) {
            if (visited.insert(c).second)
                out.push_back(c);
            c->collectAllClusters(out, false, visited);
        }
    }
}

bool isCluster(Module* m) {
    return dynamic_cast<Cluster*>(m) != nullptr;
}



// cluster.cpp
void Cluster::printCluster() const {
    printf("position: %f, %f\n", getPosition().x(), getPosition().y());
    printf("cluster: %d\n", getId());
    for (auto m : leaf) {
        printf("module: %d\n", m->getId());
        printf("position: %f, %f, rotated: %d\n", m->getPosition().x(), m->getPosition().y(), m->isRotated());
    }
}



// void Cluster::rotate()
// {
    
//     // TODO: Implement your cluster-level rotate(). (not required but strongly recommanded)
//     // Hint: First collect all submodules and subclusters with
//     //       collectAll(std::vector<Module*>& modules,
//     //                  std::vector<Cluster*>& clusters,
//     //                  /*include_this=*/true);
//     //       This lets you rotate at the cluster level without making
//     //       recursive calls to the solver.
//     Module::rotate();
    
// }


void Cluster::rotate()
{
    
    // TODO: Implement your cluster-level rotate(). (not required but strongly recommanded)
    // Hint: First collect all submodules and subclusters with
    //       collectAll(std::vector<Module*>& modules,
    //                  std::vector<Cluster*>& clusters,
    //                  /*include_this=*/true);
    //       This lets you rotate at the cluster level without making
    //       recursive calls to the solver.
    Module::rotate();
    
}
