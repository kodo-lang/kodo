#pragma once

#include <support/Assert.hh>

#include <concepts>
#include <utility>

template <typename T>
class Box {
    T *m_ptr;

public:
    template <typename... Args>
    static Box create(Args &&... args) {
        return Box(new T(std::forward<Args>(args)...));
    }

    constexpr Box() noexcept : m_ptr(nullptr) {}
    constexpr explicit Box(T *ptr) noexcept : m_ptr(ptr) {}
    Box(const Box &) = delete;
    Box(Box &&other) noexcept : m_ptr(std::exchange(other.m_ptr, nullptr)) {}
    ~Box() noexcept { delete m_ptr; }

    Box &operator=(const Box &) = delete;
    Box &operator=(T *ptr) {
        delete m_ptr;
        m_ptr = ptr;
        return *this;
    }

    template <typename U>
    Box &operator=(Box<U> &&other) noexcept requires std::derived_from<U, T> {
        *this = std::exchange(other.ptr(), nullptr);
        return *this;
    }

    T *operator*() { return m_ptr; }
    T *operator->() {
        ASSERT(m_ptr != nullptr);
        return m_ptr;
    }

    const T *operator*() const { return m_ptr; }
    const T *operator->() const {
        ASSERT(m_ptr != nullptr);
        return m_ptr;
    }

    // TODO: Remove this.
    T *&ptr() { return m_ptr; }
};
