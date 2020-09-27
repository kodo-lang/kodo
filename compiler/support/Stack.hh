#pragma once

#include <cstdint>
#include <utility>
#include <vector>

template <typename T>
class Stack {
    std::vector<T> m_impl;

public:
    constexpr void clear();

    template <typename... Args>
    constexpr void emplace(Args &&... args);

    constexpr void push(const T &value);
    constexpr T &peek();
    constexpr const T &peek() const;
    constexpr T &pop();

    constexpr bool empty() const { return m_impl.empty(); }
    constexpr std::size_t size() const { return m_impl.size(); }
};

template <typename T>
constexpr void Stack<T>::clear(){
    m_impl.clear();
}

template <typename T>
template <typename... Args>
constexpr void Stack<T>::emplace(Args &&... args) {
    m_impl.emplace_back(std::forward<Args>(args)...);
}

template <typename T>
constexpr void Stack<T>::push(const T &value) {
    m_impl.push_back(value);
}

template <typename T>
constexpr T &Stack<T>::peek() {
    return m_impl.back();
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
