#include <ir/Constants.hh>

#include <support/Assert.hh>
#include <support/PairHash.hh>

#include <unordered_map>
#include <utility>

namespace ir {
namespace {

ConstantNull s_constant_null;
std::unordered_map<std::pair<const Type *, std::size_t>, ConstantInt, PairHash> s_constant_ints;
std::unordered_map<std::string, ConstantString> s_constant_strings;

} // namespace

ConstantInt *ConstantInt::get(const Type *type, std::size_t value) {
    std::pair<const Type *, std::size_t> pair(type, value);
    if (!s_constant_ints.contains(pair)) {
        s_constant_ints.emplace(std::piecewise_construct, std::forward_as_tuple(pair),
                                std::forward_as_tuple(type, value));
    }
    return &s_constant_ints.at(pair);
}

ConstantNull *ConstantNull::get() {
    return &s_constant_null;
}

ConstantString *ConstantString::get(std::string value) {
    if (!s_constant_strings.contains(value)) {
        s_constant_strings.emplace(std::piecewise_construct, std::forward_as_tuple(value),
                                   std::forward_as_tuple(PointerType::get(IntType::get_unsigned(8)), value));
    }
    return &s_constant_strings.at(value);
}

ConstantStruct *ConstantStruct::get(const StructType *type, std::vector<Constant *> &&elems) {
    // TODO: Memory leak! Cache this.
    return new ConstantStruct(type, std::move(elems));
}

Constant *ConstantInt::clone(const Type *type) const {
    return get(type, m_value);
}

Constant *ConstantNull::clone(const Type *) const {
    ENSURE_NOT_REACHED();
}

Constant *ConstantString::clone(const Type *) const {
    ENSURE_NOT_REACHED();
}

Constant *ConstantStruct::clone(const Type *) const {
    ENSURE_NOT_REACHED();
}

} // namespace ir
