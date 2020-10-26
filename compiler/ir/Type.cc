#include <ir/Type.hh>

#include <support/PairHash.hh>

#include <memory>
#include <unordered_map>
#include <utility>

namespace ir {
namespace {

// Primitive types.
InvalidType s_invalid_type;
BoolType s_bool_type;
InferredType s_inferred_type;
VoidType s_void_type;

// Derived types.
std::unordered_map<std::pair<int, bool>, IntType, PairHash> s_int_types;
std::unordered_map<std::pair<const Type *, bool>, PointerType, PairHash> s_pointer_types;
std::vector<std::unique_ptr<StructType>> s_struct_types;

} // namespace

const InvalidType *InvalidType::get() {
    return &s_invalid_type;
}

const BoolType *BoolType::get() {
    return &s_bool_type;
}

const InferredType *InferredType::get() {
    return &s_inferred_type;
}

const IntType *IntType::get(int bit_width, bool is_signed) {
    std::pair<int, bool> pair(bit_width, is_signed);
    if (!s_int_types.contains(pair)) {
        s_int_types.emplace(std::piecewise_construct, std::forward_as_tuple(pair),
                            std::forward_as_tuple(bit_width, is_signed));
    }
    return &s_int_types.at(pair);
}

const IntType *IntType::get_signed(int bit_width) {
    return get(bit_width, true);
}

const IntType *IntType::get_unsigned(int bit_width) {
    return get(bit_width, false);
}

const PointerType *PointerType::get(const Type *pointee_type, bool is_mutable) {
    std::pair<const Type *, bool> pair(pointee_type, is_mutable);
    if (!s_pointer_types.contains(pair)) {
        s_pointer_types.emplace(std::piecewise_construct, std::forward_as_tuple(pair),
                                std::forward_as_tuple(pointee_type, is_mutable));
    }
    return &s_pointer_types.at(pair);
}

const StructType *StructType::get(std::vector<const Type *> &&fields) {
    for (const auto &type : s_struct_types) {
        if (fields.size() != type->fields().size()) {
            continue;
        }
        bool fit = true;
        for (int i = 0; i < fields.size(); i++) {
            if (fields[i] != type->fields()[i]) {
                fit = false;
                break;
            }
        }
        if (fit) {
            return type.get();
        }
    }
    return s_struct_types.emplace_back(std::make_unique<StructType>(std::move(fields))).get();
}

const VoidType *VoidType::get() {
    return &s_void_type;
}

int Type::size_in_bytes() const {
    ENSURE_NOT_REACHED();
}

std::string InvalidType::to_string() const {
    return "invalid";
}

std::string BoolType::to_string() const {
    return "bool";
}

std::string InferredType::to_string() const {
    return "inferred";
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
    for (const auto *field : m_fields) {
        size += field->size_in_bytes();
    }
    return size;
}

std::string StructType::to_string() const {
    std::string ret = "{";
    for (bool first = true; const auto *field : m_fields) {
        if (!first) {
            ret += ", ";
        }
        first = false;
        ret += field->to_string();
    }
    return ret + '}';
}

std::string VoidType::to_string() const {
    return "void";
}

} // namespace ir
