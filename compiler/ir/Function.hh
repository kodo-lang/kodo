#pragma once

#include <ir/BasicBlock.hh>
#include <support/List.hh>
#include <support/ListNode.hh>

#include <string>
#include <utility>

struct Argument : public Value, public ListNode {
    static constexpr auto KIND = ValueKind::Argument;

    Argument() : Value(KIND) {}
};

class LocalVar : public Value, public ListNode {
    const Type *const m_var_type;

public:
    static constexpr auto KIND = ValueKind::LocalVar;

    explicit LocalVar(const Type *var_type) : Value(KIND), m_var_type(var_type) {}

    const Type *var_type() const { return m_var_type; }
};

class Function : public ListNode {
    const std::string m_name;
    const Type *const m_return_type;

    // Lists must be in this order to ensure instructions are freed before arguments and local vars.
    List<Argument> m_args;
    List<LocalVar> m_vars;
    List<BasicBlock> m_blocks;

public:
    using iterator = decltype(m_blocks)::iterator;
    iterator begin() const { return m_blocks.begin(); }
    iterator end() const { return m_blocks.end(); }

    Function(std::string name, const Type *return_type) : m_name(std::move(name)), m_return_type(return_type) {}
    Function(const Function &) = delete;
    Function(Function &&) = delete;
    ~Function() override = default;

    Function &operator=(const Function &) = delete;
    Function &operator=(Function &&) = delete;

    Argument *append_arg();
    BasicBlock *append_block();
    LocalVar *append_var(const Type *type);

    const std::string &name() const { return m_name; }
    const Type *return_type() const { return m_return_type; }
    const List<Argument> &args() const { return m_args; }
    const List<LocalVar> &vars() const { return m_vars; }
    BasicBlock *entry() const;
};
