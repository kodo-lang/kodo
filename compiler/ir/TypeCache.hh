#pragma once

#include <ir/Types.hh>
#include <support/PairHash.hh>

#include <memory>
#include <unordered_map>
#include <utility>

namespace ir {

class TypeCache {
    // Primitive types.
    InvalidType m_invalid_type;
    BoolType m_bool_type;
    VoidType m_void_type;

    // Derived types.
    mutable std::unordered_map<std::pair<int, bool>, IntType, PairHash> m_int_types;
    mutable std::unordered_map<std::pair<const Type *, bool>, PointerType, PairHash> m_pointer_types;
    mutable std::vector<std::unique_ptr<StructType>> m_struct_types;

public:
    TypeCache() : m_invalid_type(this), m_bool_type(this), m_void_type(this) {}

    const InvalidType *invalid_type() const { return &m_invalid_type; }
    const BoolType *bool_type() const { return &m_bool_type; }
    const VoidType *void_type() const { return &m_void_type; }

    const IntType *int_type(int bit_width, bool is_signed) const;
    const PointerType *pointer_type(const Type *pointee, bool is_mutable) const;
    const StructType *struct_type(std::vector<StructField> &&fields) const;
};

} // namespace ir
