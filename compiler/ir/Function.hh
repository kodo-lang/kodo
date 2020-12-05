#pragma once

#include <ir/BasicBlock.hh>
#include <ir/Prototype.hh>
#include <ir/Type.hh>
#include <ir/Value.hh>
#include <support/List.hh>
#include <support/ListNode.hh>

#include <string>

namespace ir {

class FunctionType;

class Argument : public Value, public ListNode {
    const bool m_is_mutable;

public:
    static constexpr auto KIND = ValueKind::Argument;

    explicit Argument(bool is_mutable) : Value(KIND), m_is_mutable(is_mutable) {}

    bool is_mutable() const { return m_is_mutable; }
};

class LocalVar : public Value, public ListNode {
    const Type *m_var_type;

public:
    static constexpr auto KIND = ValueKind::LocalVar;

    LocalVar(const Type *var_type, bool is_mutable);

    void set_var_type(const Type *var_type);

    const Type *var_type() const { return m_var_type; }
    bool is_mutable() const;
};

class Function : public Value, public ListNode {
    const Prototype *const m_prototype;
    // Lists must be in this order to ensure instructions are freed before arguments and local vars.
    List<Argument> m_args;
    List<LocalVar> m_vars;
    List<BasicBlock> m_blocks;

public:
    static constexpr auto KIND = ValueKind::Function;
    using iterator = decltype(m_blocks)::iterator;
    iterator begin() const { return m_blocks.begin(); }
    iterator end() const { return m_blocks.end(); }

    Function(const Prototype *prototype, std::string mangled_name, const FunctionType *type);
    Function(const Function &) = delete;
    Function(Function &&) = delete;
    ~Function() override = default;

    Function &operator=(const Function &) = delete;
    Function &operator=(Function &&) = delete;

    Argument *append_arg(bool is_mutable);
    BasicBlock *append_block();
    LocalVar *append_var(const Type *type, bool is_mutable);
    void remove_var(LocalVar *var);

    const Prototype *prototype() const { return m_prototype; }
    const List<Argument> &args() const { return m_args; }
    const List<LocalVar> &vars() const { return m_vars; }
    BasicBlock *entry() const;
    const Type *return_type() const;
};

} // namespace ir
