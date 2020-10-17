#pragma once

#include <graph/DepthFirstSearch.hh>
#include <graph/DominatorTree.hh>
#include <graph/Graph.hh>
#include <support/Assert.hh>

#include <algorithm>
#include <unordered_map>
#include <utility>

template <typename V>
class DominanceComputer {
    friend Graph<V>;

protected:
    using result = DominatorTree<V>;
    result run(const Graph<V> *graph);
};

template <typename V>
typename DominanceComputer<V>::result DominanceComputer<V>::run(const Graph<V> *graph) {
    // Build post order traversal.
    // TODO: Avoid copying of post order vector.
    auto dfs = graph->template run<DepthFirstSearch>();
    auto order = dfs.post_order();

    // Build map of vertices to indices in the post order vector.
    std::unordered_map<V *, int> index_map;
    for (auto *vertex : order) {
        index_map.emplace(vertex, index_map.size());
    }

    // Transform order into reverse post order.
    std::reverse(order.begin(), order.end());

    std::unordered_map<V *, V *> doms;
    doms.emplace(graph->entry(), graph->entry());
    auto intersect = [&](V *finger1, V *finger2) {
        while (index_map.at(finger1) != index_map.at(finger2)) {
            while (index_map.at(finger1) < index_map.at(finger2)) {
                finger1 = doms.at(finger1);
            }
            while (index_map.at(finger2) < index_map.at(finger1)) {
                finger2 = doms.at(finger2);
            }
        }
        return finger1;
    };

    // Remove graph entry from order.
    ASSERT(order.front() == graph->entry());
    order.erase(order.begin());

    // TODO: Clean this up.
    bool changed = true;
    while (changed) {
        changed = false;
        for (auto *b : order) {
            V *new_idom = nullptr;
            for (auto *pred : graph->preds(b)) {
                if (!graph->preds(pred).empty() || pred == graph->entry()) {
                    new_idom = pred;
                    break;
                }
            }
            for (auto *p : graph->preds(b)) {
                if (p != new_idom && doms.contains(p)) {
                    new_idom = intersect(p, new_idom);
                }
            }
            changed = doms[b] == new_idom;
            doms[b] = new_idom;
        }
    }

    // Build idom tree.
    DominatorTree<V> tree(graph->entry());
    for (auto [vertex, idom] : doms) {
        // Ignore self-domination (non-strict).
        if (vertex != idom) {
            tree.connect(idom, vertex);
        }
    }
    return std::move(tree);
}
