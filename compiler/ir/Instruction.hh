#pragma once

#include <ir/Value.hh>
// TODO: Only include ListIterator and ListNode.
#include <support/Assert.hh>
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

    // Control flow instructions.
    Branch,
    Call,
    CondBranch,
    Phi,
    Ret,

    // Memory instructions.
    Load,
    Store,
};

class Instruction : public Value, public ListNode {
    const InstKind m_kind;
    int m_line{-1};
    BasicBlock *m_parent{nullptr};

protected:
    explicit Instruction(InstKind kind) : Value(KIND), m_kind(kind) {}

public:
    static constexpr auto KIND = ValueKind::Instruction;

    template <typename T>
    T *as() requires HasKind<T, InstKind>;
    template <typename T>
    const T *as() const requires HasKind<T, InstKind>;

    virtual void accept(Visitor *visitor) = 0;
    ListIterator<Instruction> remove_from_parent();

    bool has_parent() const;
    void set_line(int line);
    void set_parent(BasicBlock *parent);

    InstKind inst_kind() const { return m_kind; }
    int line() const { return m_line; }
    BasicBlock *parent() const { return m_parent; }
};

template <typename T>
T *Instruction::as() requires HasKind<T, InstKind> {
    ASSERT(m_kind == T::KIND);
    ASSERT_PEDANTIC(dynamic_cast<T *>(this) != nullptr);
    return static_cast<T *>(this);
}

template <typename T>
const T *Instruction::as() const requires HasKind<T, InstKind> {
    return const_cast<Instruction *>(this)->as<T>();
}

} // namespace ir
