#include <ir/Types.hh>

#include <ir/TypeCache.hh>

#include <fmt/core.h>

namespace ir {
namespace {

std::string function_type(const Type *type, bool omit_fn) {
    const auto *function_type = type->as<FunctionType>();
    const auto *return_type = function_type->return_type();
    std::string ret = !omit_fn ? "fn (" : "(";
    for (bool first = true; const auto *param : function_type->params()) {
        if (!first) {
            ret += ", ";
        }
        first = false;
        // TODO: Bit hacky.
        bool this_param = false;
        if (const auto *pointer_type = param->as_or_null<PointerType>()) {
            if (const auto *trait_type = Type::base_as<TraitType>(pointer_type->pointee_type())) {
                for (auto *prototype : trait_type->prototypes()) {
                    if (type->equals_weak(prototype->type())) {
                        this_param = true;
                        ret += "*this";
                        break;
                    }
                }
            }
        }
        if (!this_param) {
            ret += param->to_string();
        }
    }
    ret += "): " + return_type->to_string();
    return std::move(ret);
}

} // namespace

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
    case TypeKind::Trait:
        return "<anonymous trait>";
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

std::string AliasType::to_string() const {
    return m_name;
}

std::string ArrayType::to_string() const {
    return fmt::format("[{} x {}]", m_length, m_element_type->to_string());
}

std::string BoolType::to_string() const {
    return "bool";
}

std::string FunctionType::to_string() const {
    return function_type(this, false);
}

int IntType::size_in_bytes() const {
    ASSERT(m_bit_width % 8 == 0);
    return m_bit_width / 8;
}

std::string IntType::to_string() const {
    return (m_is_signed ? "i" : "u") + std::to_string(m_bit_width);
}

bool PointerType::equals_weak(const Type *other) const {
    if (Type::equals_weak(other)) {
        return true;
    }
    const auto *other_pointer = other->as_or_null<PointerType>();
    if (other_pointer == nullptr) {
        return false;
    }
    return other_pointer->is_mutable() == m_is_mutable && m_pointee_type->equals_weak(other_pointer->pointee_type());
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
    std::string ret = "struct(";
    for (bool first = true; const auto *implementing : m_implementing) {
        if (!first) {
            ret += ", ";
        }
        first = false;
        ret += implementing->to_string();
    }
    ret += ") {";
    for (bool first = true; const auto &field : m_fields) {
        if (!first) {
            ret += ", ";
        }
        first = false;
        ret += field.name() + ": ";
        ret += field.type()->to_string();
    }
    return std::move(ret + '}');
}

std::string TraitType::to_string() const {
    std::string ret = "trait {";
    for (const auto *prototype : m_prototypes) {
        ret += "\n  fn " + prototype->name() + function_type(prototype->type(), true);
    }
    if (!m_prototypes.empty()) {
        ret += '\n';
    }
    return std::move(ret + '}');
}

std::string VoidType::to_string() const {
    return "void";
}

} // namespace ir
