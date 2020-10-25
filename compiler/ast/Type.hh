#pragma once

#include <memory>
#include <string>
#include <utility>
#include <variant>
#include <vector>

namespace ast {

struct StructField;

enum class TypeKind {
    Invalid,
    Base,
    Pointer,
    Struct,
};

class Type {
    TypeKind m_kind{TypeKind::Invalid};
    std::variant<std::string, std::pair<std::unique_ptr<Type>, bool>, std::vector<StructField>> m_data;

    template <typename T>
    Type(TypeKind kind, T data) : m_kind(kind), m_data(std::move(data)) {}

public:
    static Type get_base(std::string &&base);
    static Type get_pointer(Type &&pointee, bool is_mutable);
    static Type get_struct(std::vector<StructField> &&fields);

    Type() = default;

    TypeKind kind() const { return m_kind; }
    const std::string &base() const { return std::get<std::string>(m_data); }
    const Type &pointee() const { return *std::get<std::pair<std::unique_ptr<Type>, bool>>(m_data).first; }
    bool is_mutable() const { return std::get<std::pair<std::unique_ptr<Type>, bool>>(m_data).second; }
    const std::vector<StructField> &struct_fields() const { return std::get<std::vector<StructField>>(m_data); }
};

struct StructField {
    std::string name;
    Type type;
};

} // namespace ast
