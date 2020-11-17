#pragma once

#include <ir/Type.hh>
#include <support/Assert.hh>
#include <support/Castable.hh>

#include <string>
#include <vector>

namespace ir {

enum class ValueKind {
    Argument,
    BasicBlock,
    Constant,
    Instruction,
    LocalVar,
    MemoryPhi,
};

class Value : public Castable<Value, ValueKind, true> {
    const ValueKind m_kind;
    const Type *m_type{nullptr};
    std::string m_name;
    // TODO: Consider small vector optimisation.
    std::vector<Value *> m_users;

protected:
    explicit Value(ValueKind kind) : m_kind(kind) {}

public:
    Value(const Value &) = delete;
    Value(Value &&) = delete;
    virtual ~Value();

    Value &operator=(const Value &) = delete;
    Value &operator=(Value &&) = delete;

    void add_user(Value *user);
    void remove_user(Value *user);
    void replace_all_uses_with(Value *repl);
    virtual void replace_uses_of_with(Value *orig, Value *repl);

    bool has_type() const;
    void set_type(const Type *type);

    bool has_name() const;
    void set_name(std::string name);

    ValueKind kind() const { return m_kind; }
    const Type *type() const { return m_type; }
    const std::string &name() const { return m_name; }
    const std::vector<Value *> &users() const { return m_users; }
};

} // namespace ir
