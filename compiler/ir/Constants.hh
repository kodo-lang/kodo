#pragma once

#include <ir/Constant.hh>

#include <cstdint>

namespace ir {

class ConstantInt : public Constant {
    const std::size_t m_value;

public:
    static constexpr auto KIND = ConstantKind::Int;
    static ConstantInt *get(const Type *type, std::size_t value);

    ConstantInt(const Type *type, std::size_t value) : Constant(KIND, type), m_value(value) {}

    Constant *clone(const Type *type) const override;

    std::size_t value() const { return m_value; }
};

struct ConstantNull : public Constant {
    static constexpr auto KIND = ConstantKind::Null;
    static ConstantNull *get();

    ConstantNull() noexcept : Constant(KIND, nullptr) {}

    Constant *clone(const Type *type) const override;
};

} // namespace ir
