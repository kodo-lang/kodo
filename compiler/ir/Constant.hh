#pragma once

#include <ir/Value.hh>

#include <cstdint>

class Constant : public Value {
    const std::uint64_t m_value;

public:
    static constexpr auto KIND = ValueKind::Constant;

    explicit Constant(std::uint64_t value) : Value(KIND), m_value(value) {}

    std::uint64_t value() const { return m_value; }
};
