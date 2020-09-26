#include <ir/Value.hh>

#include <algorithm>
#include <cassert>
#include <utility>

Value::~Value() {
    replace_all_uses_with(nullptr);
}

bool Value::is(ValueKind kind) const {
    return m_kind == kind;
}

void Value::add_user(Value *user) {
    m_users.push_back(user);
}

void Value::remove_user(Value *user) {
    // TODO: Is remove_user() ever called when the user isn't present? Is the check necessary?
    auto it = std::find(m_users.begin(), m_users.end(), user);
    if (it != m_users.end()) {
        m_users.erase(it);
    }
}

void Value::replace_all_uses_with(Value *repl) {
    for (auto *user : m_users) {
        user->replace_uses_of_with(this, repl);
    }
    // Make sure that all users have removed themselves as users of us.
    assert(m_users.empty());
}

void Value::replace_uses_of_with(Value *, Value *) {
    // Should be implemented by subclass.
    assert(false);
}

bool Value::has_name() const {
    return !m_name.empty();
}

void Value::set_name(std::string name) {
    m_name = std::move(name);
}

bool Value::has_type() const {
    return m_type != nullptr;
}

void Value::set_type(const Type *type) {
    m_type = type;
}
