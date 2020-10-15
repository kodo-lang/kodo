#pragma once

#include <graph/Graph.hh>

#include <unordered_map>
#include <utility>
#include <vector>

template <typename V>
class DepthFirstSearch {
    friend Graph<V>;

private:
    enum class State {
        Unexplored = 0,
        Exploring,
        Explored,
    };

    std::vector<V *> m_pre_order;
    std::vector<V *> m_post_order;

    void dfs(const Graph<V> *graph, std::unordered_map<V *, State> &state, V *vertex);

protected:
    using result = DepthFirstSearch<V>;
    result run(const Graph<V> *graph);

public:
    const std::vector<V *> &pre_order() const { return m_pre_order; }
    const std::vector<V *> &post_order() const { return m_post_order; }
};

template <typename V>
void DepthFirstSearch<V>::dfs(const Graph<V> *graph, std::unordered_map<V *, State> &state, V *vertex) {
    m_pre_order.push_back(vertex);
    for (auto *succ : graph->succs(vertex)) {
        if (state[succ] != State::Unexplored) {
            continue;
        }
        state[succ] = State::Exploring;
        dfs(graph, state, succ);
    }
    state[vertex] = State::Explored;
    m_post_order.push_back(vertex);
}

template <typename V>
typename DepthFirstSearch<V>::result DepthFirstSearch<V>::run(const Graph<V> *graph) {
    std::unordered_map<V *, State> state;
    state.emplace(graph->entry(), State::Exploring);
    dfs(graph, state, graph->entry());
    return std::move(*this);
}
