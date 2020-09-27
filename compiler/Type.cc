#include <Type.hh>

#include <unordered_map>

namespace {

std::unordered_map<int, IntType> s_int_types;
std::unordered_map<const Type *, PointerType> s_pointer_types;

} // namespace

const IntType *IntType::get(int bit_width) {
    if (!s_int_types.contains(bit_width)) {
        s_int_types.emplace(bit_width, bit_width);
    }
    return &s_int_types.at(bit_width);
}

const PointerType *PointerType::get(const Type *pointee_type) {
    if (!s_pointer_types.contains(pointee_type)) {
        s_pointer_types.emplace(pointee_type, pointee_type);
    }
    return &s_pointer_types.at(pointee_type);
}

// TODO: Just compare types by pointer.
bool operator==(const Type &lhs, const Type &rhs) {
    if (lhs.kind() != rhs.kind()) {
        return false;
    }
    switch (lhs.kind()) {
    case TypeKind::Invalid:
        return true;
    case TypeKind::Int:
        return lhs.as<IntType>()->bit_width() == rhs.as<IntType>()->bit_width();
    case TypeKind::Pointer:
        return *lhs.as<PointerType>()->pointee_type() == *rhs.as<PointerType>()->pointee_type();
    }
}
