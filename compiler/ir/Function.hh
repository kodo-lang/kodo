#pragma once

#include <ir/BasicBlock.hh>
#include <support/List.hh>
#include <support/ListNode.hh>

#include <string>
#include <utility>

namespace ir {

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

    LocalVar(const Type *var_type, bool is_mutable) : Value(KIND), m_var_type(var_type) {
        set_type(PointerType::get(var_type, is_mutable));
    }

    void set_var_type(const Type *var_type) {
        m_var_type = var_type;
        set_type(PointerType::get(m_var_type, is_mutable()));
    }

    const Type *var_type() const { return m_var_type; }
    bool is_mutable() const { return type()->as<PointerType>()->is_mutable(); }
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

    Argument *append_arg(bool is_mutable);
    BasicBlock *append_block();
    LocalVar *append_var(const Type *type, bool is_mutable);

    const std::string &name() const { return m_name; }
    const Type *return_type() const { return m_return_type; }
    const List<Argument> &args() const { return m_args; }
    const List<LocalVar> &vars() const { return m_vars; }
    BasicBlock *entry() const;
};

} // namespace ir
