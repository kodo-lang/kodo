#pragma once

#include <string>
#include <utility>

namespace ast {

class Type {
    std::string m_base;
    int m_pointer_levels{0};

public:
    Type() = default;
    Type(std::string base, int pointer_levels) : m_base(std::move(base)), m_pointer_levels(pointer_levels) {}
    Type(const Type &) = delete;
    Type(Type &&) noexcept = default;
    ~Type() = default;

    Type &operator=(const Type &) = delete;
    Type &operator=(Type &&) noexcept = default;

    const std::string &base() const { return m_base; }
    int pointer_levels() const { return m_pointer_levels; }
};

} // namespace ast
