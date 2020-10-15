#pragma once

#include <graph/Graph.hh>
#include <support/Assert.hh>

template <typename V>
struct DominatorTree : public Graph<V> {
    explicit DominatorTree(V *entry) : Graph<V>(entry) {}
    V *idom(const V *vertex) const;
};

template <typename V>
V *DominatorTree<V>::idom(const V *vertex) const {
    if (vertex == this->entry()) {
        return nullptr;
    }
    ASSERT(this->preds(vertex).size() == 1);
    return this->preds(vertex)[0];
}
