#include <Type.hh>

#include <unordered_map>

namespace {

InvalidType s_invalid_type;
std::unordered_map<int, IntType> s_int_types;
std::unordered_map<const Type *, PointerType> s_pointer_types;

} // namespace

const InvalidType *InvalidType::get() {
    return &s_invalid_type;
}

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
