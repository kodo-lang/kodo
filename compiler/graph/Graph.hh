#pragma once

#include <algorithm>
#include <unordered_map>
#include <vector>

template <typename V>
class Graph {
    std::unordered_map<const V *, std::vector<V *>> m_preds;
    std::unordered_map<const V *, std::vector<V *>> m_succs;
    V *m_entry;

public:
    explicit Graph(V *entry) : m_entry(entry) {}
    Graph(const Graph &) = delete;
    Graph(Graph &&) noexcept = default;
    ~Graph() = default;

    Graph &operator=(const Graph &) = delete;
    Graph &operator=(Graph &&) noexcept = default;

    void connect(V *src, V *dst);
    void disconnect(V *src, V *dst);

    template <template <typename> typename T, typename U = T<V>>
    typename U::result run() const {
        return U().run(this);
    }

    const std::vector<V *> &preds(const V *vertex) const;
    const std::vector<V *> &succs(const V *vertex) const;

    V *entry() const { return m_entry; }
};

template <typename V>
void Graph<V>::connect(V *src, V *dst) {
    m_preds[dst].push_back(src);
    m_succs[src].push_back(dst);
}

template <typename V>
void Graph<V>::disconnect(V *src, V *dst) {
    auto &pred_vec = m_preds.at(dst);
    auto &succ_vec = m_succs.at(src);
    auto pred_it = std::find(pred_vec.begin(), pred_vec.end(), src);
    auto succ_it = std::find(succ_vec.begin(), succ_vec.end(), dst);
    pred_vec.erase(pred_it);
    succ_vec.erase(succ_it);
}

template <typename V>
const std::vector<V *> &Graph<V>::preds(const V *vertex) const {
    return const_cast<Graph<V> *>(this)->m_preds[vertex];
}

template <typename V>
const std::vector<V *> &Graph<V>::succs(const V *vertex) const {
    return const_cast<Graph<V> *>(this)->m_succs[vertex];
}
