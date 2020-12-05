#include <ir/Types.hh>

namespace ir {

const Type *Type::base(const Type *type) {
    if (const auto *alias = type->as_or_null<AliasType>()) {
        return alias->aliased();
    }
    return type;
}

std::string Type::name(const Type *type) {
    switch (type->kind()) {
    case TypeKind::Alias:
        return type->as<AliasType>()->name();
    case TypeKind::Struct:
        return "<anonymous struct>";
    default:
        ENSURE_NOT_REACHED();
    }
}

bool Type::equals_weak(const Type *other) const {
    return other == this;
}

int Type::size_in_bytes() const {
    ENSURE_NOT_REACHED();
}

std::string InvalidType::to_string() const {
    return "invalid";
}

bool AliasType::equals_weak(const Type *other) const {
    return Type::equals_weak(other) || m_aliased->equals_weak(other);
}

std::string BoolType::to_string() const {
    return "bool";
}

std::string FunctionType::to_string() const {
    std::string ret = "(";
    for (bool first = true; const auto *param : m_params) {
        if (!first) {
            ret += ", ";
        }
        first = false;
        ret += param->to_string();
    }
    ret += "): " + m_return_type->to_string();
    return std::move(ret);
}

int IntType::size_in_bytes() const {
    ASSERT(m_bit_width % 8 == 0);
    return m_bit_width / 8;
}

std::string IntType::to_string() const {
    return (m_is_signed ? "i" : "u") + std::to_string(m_bit_width);
}

std::string PointerType::to_string() const {
    std::string ret = "*";
    ret += m_is_mutable ? "mut " : "";
    ret += m_pointee_type->to_string();
    return std::move(ret);
}

int StructType::size_in_bytes() const {
    int size = 0;
    for (const auto &field : m_fields) {
        size += field.type()->size_in_bytes();
    }
    return size;
}

std::string StructType::to_string() const {
    std::string ret = "struct {";
    for (bool first = true; const auto &field : m_fields) {
        if (!first) {
            ret += ", ";
        }
        first = false;
        ret += field.name() + ": ";
        ret += field.type()->to_string();
    }
    return ret + '}';
}

std::string VoidType::to_string() const {
    return "void";
}

} // namespace ir
