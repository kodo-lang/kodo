#pragma once

#include <string>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <vector>

// TODO: Add tests for the parser.
namespace args {

template <typename T>
class Value {
    bool m_present{false};
    bool m_value_passed{false};
    T m_value;

public:
    explicit Value(T value) : m_value(value) {}

    void set_present(bool present) { m_present = present; }
    void set_value_passed(bool value_passed) { m_value_passed = value_passed; }
    void set_value(T value) { m_value = value; }

    bool present_or_true() const requires std::is_same_v<T, bool> { return (m_present && !m_value_passed) || m_value; }

    bool present() const { return m_present; }
    const T &value() const { return m_value; }
};

class Option {
    const std::string m_name;
    Value<bool> *const m_value{nullptr};

public:
    template <typename T>
    Option(std::string name, Value<T> *value)
        : m_name(std::move(name)), m_value(reinterpret_cast<Value<bool> *>(value)) {}

    const std::string &name() const { return m_name; }
    Value<bool> *value() const { return m_value; }
};

class Parser {
    std::vector<std::string *> m_args;
    std::vector<Option> m_options;

public:
    void add_arg(std::string *arg) { m_args.push_back(arg); }

    template <typename... Args>
    Option &add_option(Args &&... args) {
        return m_options.emplace_back(std::forward<Args>(args)...);
    }

    void parse(int argc, char **argv);
};

} // namespace args
