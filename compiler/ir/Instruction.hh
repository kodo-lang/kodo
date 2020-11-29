#pragma once

#include <ir/Value.hh>
// TODO: Only include ListIterator and ListNode.
#include <support/Assert.hh>
#include <support/Castable.hh>
#include <support/List.hh>
#include <support/ListNode.hh>

namespace ir {

class BasicBlock;
class Visitor;

enum class InstKind {
    // Arithmetic/Logic instructions.
    Binary,
    Cast,
    Compare,
    InlineAsm,

    // Control flow instructions.
    Branch,
    Call,
    CondBranch,
    Phi,
    Ret,

    // Memory instructions.
    Copy,
    Lea,
    Load,
    Store,
};

class Instruction : public Value, public ListNode {
    const InstKind m_kind;
    BasicBlock *const m_parent;
    int m_line{-1};

protected:
    Instruction(InstKind kind, BasicBlock *parent) : Value(KIND), m_kind(kind), m_parent(parent) {}

public:
    static constexpr auto KIND = ValueKind::Instruction;

    // TODO: Remove code duplication from Castable.
    template <typename T>
    T *as() requires HasKind<T, InstKind>;
    template <typename T>
    T *as_or_null() requires HasKind<T, InstKind>;

    template <typename T>
    const T *as() const requires HasKind<T, InstKind>;
    template <typename T>
    const T *as_or_null() const requires HasKind<T, InstKind>;

    virtual void accept(Visitor *visitor) = 0;
    ListIterator<Instruction> remove_from_parent();

    void set_line(int line);

    InstKind kind() const { return m_kind; }
    BasicBlock *parent() const { return m_parent; }
    int line() const { return m_line; }
};

template <typename T>
T *Instruction::as() requires HasKind<T, InstKind> {
    ASSERT(m_kind == T::KIND);
    ASSERT_PEDANTIC(dynamic_cast<T *>(this) != nullptr);
    return static_cast<T *>(this);
}

template <typename T>
T *Instruction::as_or_null() requires HasKind<T, InstKind> {
    return m_kind == T::KIND ? as<T>() : nullptr;
}

template <typename T>
const T *Instruction::as() const requires HasKind<T, InstKind> {
    return const_cast<Instruction *>(this)->as<T>();
}

template <typename T>
const T *Instruction::as_or_null() const requires HasKind<T, InstKind> {
    return const_cast<Instruction *>(this)->as_or_null<T>();
}

} // namespace ir
