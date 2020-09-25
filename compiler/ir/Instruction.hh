#pragma once

#include <ir/Value.hh>
// TODO: Only include ListIterator and ListNode.
#include <support/List.hh>
#include <support/ListNode.hh>

class BasicBlock;
class Visitor;

enum class InstKind {
    // Arithmetic/Logic instructions.
    Binary,
    Compare,

    // Control flow instructions.
    Branch,
    CondBranch,
    Phi,
    Ret,

    // Memory instructions.
    Load,
    Store,
};

class Instruction : public Value, public ListNode {
    const InstKind m_kind;
    BasicBlock *m_parent{nullptr};

protected:
    explicit Instruction(InstKind kind) : Value(KIND), m_kind(kind) {}

public:
    static constexpr auto KIND = ValueKind::Instruction;

    template <typename T>
    T *as() requires HasKind<T, InstKind>;

    template <typename T>
    const T *as() const requires HasKind<T, InstKind>;

    bool is(InstKind kind) const;

    virtual void accept(Visitor *visitor) = 0;
    ListIterator<Instruction> remove_from_parent();

    bool has_parent() const;
    void set_parent(BasicBlock *parent);

    InstKind inst_kind() const { return m_kind; }
    BasicBlock *parent() const { return m_parent; }
};

template <typename T>
T *Instruction::as() requires HasKind<T, InstKind> {
    return is(T::KIND) ? static_cast<T *>(this) : nullptr;
}

template <typename T>
const T *Instruction::as() const requires HasKind<T, InstKind> {
    return is(T::KIND) ? static_cast<const T *>(this) : nullptr;
}
