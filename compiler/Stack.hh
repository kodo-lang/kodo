#pragma once

#include <cstdint>
#include <vector>

template <typename T>
class Stack {
    std::vector<T> m_impl;

public:
    constexpr void push(const T &value);
    constexpr const T &peek() const;
    constexpr T &pop();

    constexpr bool empty() const { return m_impl.empty(); }
    constexpr std::size_t size() const { return m_impl.size(); }
};

template <typename T>
constexpr void Stack<T>::push(const T &value) {
    m_impl.push_back(value);
}

template <typename T>
constexpr const T &Stack<T>::peek() const {
    return m_impl.back();
}

template <typename T>
constexpr T &Stack<T>::pop() {
    auto &ret = m_impl.back();
    m_impl.pop_back();
    return ret;
}
