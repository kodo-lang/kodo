#pragma once

#include <ir/Constant.hh>

#include <cstdint>
#include <string>
#include <utility>

namespace ir {

// TODO: Hide constructors?
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

class ConstantString : public Constant {
    const std::string m_value;

public:
    static constexpr auto KIND = ConstantKind::String;
    static ConstantString *get(std::string value);

    ConstantString(const Type *type, std::string value) : Constant(KIND, type), m_value(std::move(value)) {}

    Constant *clone(const Type *type) const override;

    const std::string &value() const { return m_value; }
};

} // namespace ir
