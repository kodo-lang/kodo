#include <ir/TypeCache.hh>

namespace ir {

const AliasType *TypeCache::alias_type(const Type *aliased, std::string &&name) const {
    for (const auto &type : m_alias_types) {
        if (type->aliased() == aliased && type->name() == name) {
            return *type;
        }
    }
    return *m_alias_types.emplace_back(new AliasType(this, aliased, std::move(name)));
}

const ArrayType *TypeCache::array_type(const Type *element_type, std::size_t length) const {
    std::pair<const Type *, std::size_t> pair(element_type, length);
    if (!m_array_types.contains(pair)) {
        m_array_types.emplace(std::piecewise_construct, std::forward_as_tuple(pair),
                              std::forward_as_tuple(this, element_type, length));
    }
    return &m_array_types.at(pair);
}

const FunctionType *TypeCache::function_type(const Type *return_type, std::vector<const Type *> &&params) const {
    for (const auto &type : m_function_types) {
        if (type->return_type() == return_type && type->params() == params) {
            return *type;
        }
    }
    return *m_function_types.emplace_back(new FunctionType(this, return_type, std::move(params)));
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

} // namespace ir
