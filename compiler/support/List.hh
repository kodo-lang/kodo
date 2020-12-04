#pragma once

#include <support/Box.hh>
#include <support/ListNode.hh>

#include <concepts>
#include <type_traits>
#include <utility>
#include <vector>

// TODO: Remove clang-format disable comments :(.

// clang-format off
template <typename T> requires std::derived_from<T, ListNode>
class List;
// clang-format on

template <typename T>
class ListIterator {
    ListNode *m_elem;

public:
    explicit ListIterator(ListNode *elem) : m_elem(elem) {}

    std::strong_ordering operator<=>(const ListIterator &) const = default;
    T *operator*() const { return static_cast<T *>(m_elem); }
    T *operator->() const { return static_cast<T *>(m_elem); }
    ListIterator operator++(int) { return ListIterator(m_elem->next()); }
    ListIterator &operator--() {
        m_elem = m_elem->prev();
        return *this;
    }
    ListIterator &operator++() {
        m_elem = m_elem->next();
        return *this;
    }
};

// clang-format off
template <typename T> requires std::derived_from<T, ListNode>
class List {
    // clang-format on
    // Store ListNode here to allow for abstract Ts.
    Box<ListNode> m_end;

public:
    using iterator = ListIterator<T>;

    List();
    List(const List &) = delete;
    List(List &&) noexcept = default;
    ~List();

    List &operator=(const List &) = delete;
    List &operator=(List &&) noexcept = default;

    template <typename U, typename... Args>
    U *emplace(iterator it, Args &&... args) requires std::derived_from<U, T>;
    void insert(iterator it, T *elem);
    iterator erase(iterator it);

    T *operator[](std::size_t n);
    const T *operator[](std::size_t n) const;
    bool operator==(const List &other) const;

    bool empty() const;
    int size() const;

    // TODO: RIP const-correctness. Need proper iterators (const and non-const variants).
    iterator begin() const { return ++end(); }
    iterator end() const { return iterator(*const_cast<List *>(this)->m_end); }
};

// clang-format off
template <typename T> requires std::derived_from<T, ListNode>
List<T>::List() {
    // clang-format on
    m_end = Box<ListNode>::create();
    m_end->set_prev(*m_end);
    m_end->set_next(*m_end);
}

// clang-format off
template <typename T> requires std::derived_from<T, ListNode>
List<T>::~List() {
    // clang-format on
    if (*m_end == nullptr) {
        return;
    }
    for (auto it = begin(); *it != *m_end;) {
        auto *elem = *it;
        ++it;
        delete elem;
    }
}

// clang-format off
template <typename T> requires std::derived_from<T, ListNode>
template <typename U, typename... Args>
U *List<T>::emplace(iterator it, Args &&... args) requires std::derived_from<U, T> {
    // clang-format on
    auto *elem = new U(std::forward<Args>(args)...);
    insert(it, elem);
    return elem;
}

// clang-format off
template <typename T> requires std::derived_from<T, ListNode>
void List<T>::insert(iterator it, T *elem) {
    // clang-format on
    auto *prev = it->prev();
    elem->set_prev(prev);
    elem->set_next(*it);
    it->set_prev(elem);
    prev->set_next(elem);
}

// clang-format off
template <typename T> requires std::derived_from<T, ListNode>
typename List<T>::iterator List<T>::erase(iterator it) {
    // clang-format on
    auto *prev = it->prev();
    auto *next = it->next();
    next->set_prev(prev);
    prev->set_next(next);

    auto ret = it++;
    delete *it;
    return ret;
}

// clang-format off
template <typename T> requires std::derived_from<T, ListNode>
T *List<T>::operator[](std::size_t n) {
    // clang-format on
    auto it = begin();
    std::advance(it, n);
    return *it;
}

// clang-format off
template <typename T> requires std::derived_from<T, ListNode>
const T *List<T>::operator[](std::size_t n) const {
    // clang-format on
    auto it = begin();
    std::advance(it, n);
    return *it;
}

// clang-format off
template <typename T> requires std::derived_from<T, ListNode>
bool List<T>::operator==(const List &other) const {
    // clang-format on
    if (size() != other.size()) {
        return false;
    }
    for (int i = 0; i < size(); i++) {
        if ((*this)[i] != other[i]) {
            return false;
        }
    }
    return true;
}

// clang-format off
template <typename T> requires std::derived_from<T, ListNode>
bool List<T>::empty() const {
    // clang-format on
    return size() == 0;
}

// clang-format off
template <typename T> requires std::derived_from<T, ListNode>
int List<T>::size() const {
    // clang-format on
    return std::distance(begin(), end());
}

namespace std {

template <typename T>
struct iterator_traits<ListIterator<T>> {
    using difference_type = int;
    using iterator_category = std::forward_iterator_tag;
};

} // namespace std
