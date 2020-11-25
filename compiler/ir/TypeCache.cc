#include <ir/TypeCache.hh>

namespace ir {

const FunctionType *TypeCache::function_type(const Type *return_type, std::vector<const Type *> &&params) const {
    for (const auto &type : m_function_types) {
        if (type->return_type() == return_type && type->params() == params) {
            return type.get();
        }
    }
    return m_function_types.emplace_back(std::make_unique<FunctionType>(this, return_type, std::move(params))).get();
}

const IntType *TypeCache::int_type(int bit_width, bool is_signed) const {
    std::pair<int, bool> pair(bit_width, is_signed);
    if (!m_int_types.contains(pair)) {
        m_int_types.emplace(std::piecewise_construct, std::forward_as_tuple(pair),
                            std::forward_as_tuple(this, bit_width, is_signed));
    }
    return &m_int_types.at(pair);
}

const PointerType *TypeCache::pointer_type(const Type *pointee, bool is_mutable) const {
    std::pair<const Type *, bool> pair(pointee, is_mutable);
    if (!m_pointer_types.contains(pair)) {
        m_pointer_types.emplace(std::piecewise_construct, std::forward_as_tuple(pair),
                                std::forward_as_tuple(this, pointee, is_mutable));
    }
    return &m_pointer_types.at(pair);
}

const StructType *TypeCache::struct_type(std::vector<StructField> &&fields) const {
    for (const auto &type : m_struct_types) {
        if (type->fields() == fields) {
            return type.get();
        }
    }
    // TODO: Remove unique_ptr.
    return m_struct_types.emplace_back(std::make_unique<StructType>(this, std::move(fields))).get();
}

} // namespace ir
