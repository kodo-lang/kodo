#pragma once

#include <Type.hh>
#include <support/HasKind.hh>

#include <cassert>
#include <string>
#include <vector>

enum class ValueKind {
    Argument,
    BasicBlock,
    Constant,
    Instruction,
    LocalVar,
};

class Value {
    const ValueKind m_kind;
    const Type *m_type{nullptr};
    std::string m_name;
    // TODO: Consider small vector optimisation.
    std::vector<Value *> m_users;

protected:
    explicit Value(ValueKind kind);

public:
    Value(const Value &) = delete;
    Value(Value &&) = delete;
    virtual ~Value();

    Value &operator=(const Value &) = delete;
    Value &operator=(Value &&) = delete;

    template <typename T>
    T *as() requires HasKind<T, ValueKind>;
    template <typename T>
    T *as_or_null() requires HasKind<T, ValueKind>;

    template <typename T>
    const T *as() const requires HasKind<T, ValueKind>;
    template <typename T>
    const T *as_or_null() const requires HasKind<T, ValueKind>;

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

template <typename T>
T *Value::as() requires HasKind<T, ValueKind> {
    // TODO: dynamic_cast check worth it?
    assert(m_kind == T::KIND);
    assert(dynamic_cast<T *>(this) != nullptr);
    return static_cast<T *>(this);
}

template <typename T>
T *Value::as_or_null() requires HasKind<T, ValueKind> {
    return m_kind == T::KIND ? as<T>() : nullptr;
}

template <typename T>
const T *Value::as() const requires HasKind<T, ValueKind> {
    return const_cast<Value *>(this)->as<T>();
}

template <typename T>
const T *Value::as_or_null() const requires HasKind<T, ValueKind> {
    return const_cast<Value *>(this)->as_or_null<T>();
}
