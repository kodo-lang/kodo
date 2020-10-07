#include <Type.hh>

#include <support/PairHash.hh>

#include <unordered_map>
#include <utility>

namespace {

// Primitive types.
InvalidType s_invalid_type;
BoolType s_bool_type;
VoidType s_void_type;

// Derived types.
std::unordered_map<std::pair<int, bool>, IntType, PairHash> s_int_types;
std::unordered_map<const Type *, PointerType> s_pointer_types;

} // namespace

const InvalidType *InvalidType::get() {
    return &s_invalid_type;
}

const BoolType *BoolType::get() {
    return &s_bool_type;
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

const PointerType *PointerType::get(const Type *pointee_type) {
    if (!s_pointer_types.contains(pointee_type)) {
        s_pointer_types.emplace(pointee_type, pointee_type);
    }
    return &s_pointer_types.at(pointee_type);
}

const VoidType *VoidType::get() {
    return &s_void_type;
}

std::string InvalidType::to_string() const {
    return "invalid";
}

std::string BoolType::to_string() const {
    return "bool";
}

std::string IntType::to_string() const {
    return (m_is_signed ? "i" : "u") + std::to_string(m_bit_width);
}

std::string PointerType::to_string() const {
    return m_pointee_type->to_string() + "*";
}

std::string VoidType::to_string() const {
    return "void";
}
