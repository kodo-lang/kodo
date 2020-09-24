#pragma once

#include <ir/BasicBlock.hh>
#include <support/List.hh>
#include <support/ListNode.hh>

#include <string>
#include <utility>

struct Argument : public Value, public ListNode {
    Argument() : Value(ValueKind::Argument) {}
};

struct LocalVar : public Value, public ListNode {
    LocalVar() : Value(ValueKind::LocalVar) {}
};

class Function : public ListNode {
    const std::string m_name;
    List<Argument> m_args;
    List<BasicBlock> m_blocks;
    List<LocalVar> m_vars;

public:
    using iterator = decltype(m_blocks)::iterator;
    iterator begin() const { return m_blocks.begin(); }
    iterator end() const { return m_blocks.end(); }

    explicit Function(std::string name) : m_name(std::move(name)) {}
    Function(const Function &) = delete;
    Function(Function &&) = delete;
    ~Function() override = default;

    Function &operator=(const Function &) = delete;
    Function &operator=(Function &&) = delete;

    Argument *append_arg();
    BasicBlock *append_block();
    LocalVar *append_var();

    const std::string &name() const { return m_name; }
    const List<Argument> &args() const { return m_args; }
    const List<LocalVar> &vars() const { return m_vars; }
    BasicBlock *entry() const;
};
