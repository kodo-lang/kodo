#pragma once

#include <ir/Value.hh>
#include <support/Assert.hh>
#include <support/Castable.hh>

namespace ir {

enum class ConstantKind {
    Array,
    Int,
    Null,
    String,
    Undef,
};

class Constant : public Value {
    const ConstantKind m_kind;

protected:
    Constant(ConstantKind kind, const Type *type) : Value(KIND), m_kind(kind) { set_type(type); }

public:
    static constexpr auto KIND = ValueKind::Constant;

    // TODO: Remove code duplication from Castable.
    template <typename T>
    const T *as() const requires HasKind<T, ConstantKind>;
    template <typename T>
    const T *as_or_null() const requires HasKind<T, ConstantKind>;

    virtual Constant *clone(const Type *type) const = 0;

    ConstantKind kind() const { return m_kind; }
};

template <typename T>
const T *Constant::as() const requires HasKind<T, ConstantKind> {
    ASSERT(m_kind == T::KIND);
    ASSERT_PEDANTIC(dynamic_cast<const T *>(this) != nullptr);
    return static_cast<const T *>(this);
}

template <typename T>
const T *Constant::as_or_null() const requires HasKind<T, ConstantKind> {
    return m_kind == T::KIND ? as<T>() : nullptr;
}

} // namespace ir
