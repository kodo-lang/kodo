#pragma once

#include <string>
#include <vector>

template <typename T, typename Kind>
concept HasKind = requires(T t) {
    static_cast<Kind>(T::KIND);
};

enum class ValueKind {
    BasicBlock,
    Constant,
    Instruction,
};

// TODO: Kind enum + static_cast for casting instead of dynamic_cast (like type system).
class Value {
    const ValueKind m_kind;
    std::string m_name;
    // TODO: Consider small vector optimisation.
    std::vector<Value *> m_users;

protected:
    explicit Value(ValueKind kind) : m_kind(kind) {}

public:
    Value(const Value &) = delete;
    Value(Value &&) = delete;
    virtual ~Value() = default;

    Value &operator=(const Value &) = delete;
    Value &operator=(Value &&) = delete;

    template <typename T>
    T *as() requires HasKind<T, ValueKind>;

    template <typename T>
    const T *as() const requires HasKind<T, ValueKind>;

    bool is(ValueKind kind) const;

    void add_user(Value *user);
    void remove_user(Value *user);
    void replace_all_uses_with(Value *repl);
    virtual void replace_uses_of_with(Value *orig, Value *repl);

    bool has_name() const;
    void set_name(std::string name);

    const std::string &name() const { return m_name; }
    const std::vector<Value *> &users() const { return m_users; }
};

template <typename T>
T *Value::as() requires HasKind<T, ValueKind> {
    return is(T::KIND) ? static_cast<T *>(this) : nullptr;
}

template <typename T>
const T *Value::as() const requires HasKind<T, ValueKind> {
    return is(T::KIND) ? static_cast<const T *>(this) : nullptr;
}
