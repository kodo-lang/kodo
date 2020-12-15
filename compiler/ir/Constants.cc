#include <ir/Constants.hh>

#include <ir/Program.hh>
#include <ir/Type.hh>
#include <support/Assert.hh>
#include <support/PairHash.hh>

#include <unordered_map>
#include <utility>
#include <vector>

namespace ir {
namespace {

std::vector<Box<ConstantArray>> s_constant_arrays;
std::unordered_map<std::pair<const Type *, std::size_t>, ConstantInt, PairHash> s_constant_ints;
std::unordered_map<const Type *, ConstantNull> s_constant_nulls;
std::unordered_map<std::string, ConstantString> s_constant_strings;
std::unordered_map<const Type *, Undef> s_undefs;

} // namespace

ConstantArray *ConstantArray::get(const ArrayType *type, std::vector<Value *> &&elems) {
    ASSERT(type->length() == elems.size());
    for (auto &constant_array : s_constant_arrays) {
        if (constant_array->type() == type && constant_array->elems() == elems) {
            return *constant_array;
        }
    }
    return *s_constant_arrays.emplace_back(new ConstantArray(type, std::move(elems)));
}

ConstantArray *ConstantArray::get(std::vector<Value *> &&elems) {
    ASSERT(!elems.empty());
    const auto *element_type = elems[0]->type();
    const auto *type = element_type->cache()->array_type(element_type, elems.size());
    return get(type, std::move(elems));
}

ConstantInt *ConstantInt::get(const Type *type, std::size_t value) {
    std::pair<const Type *, std::size_t> pair(type, value);
    if (!s_constant_ints.contains(pair)) {
        s_constant_ints.emplace(std::piecewise_construct, std::forward_as_tuple(pair),
                                std::forward_as_tuple(type, value));
    }
    return &s_constant_ints.at(pair);
}

ConstantNull *ConstantNull::get(const Type *type) {
    if (!s_constant_nulls.contains(type)) {
        s_constant_nulls.emplace(type, type);
    }
    return &s_constant_nulls.at(type);
}

ConstantString *ConstantString::get(const Program *program, std::string value) {
    if (!s_constant_strings.contains(value)) {
        s_constant_strings.emplace(
            std::piecewise_construct, std::forward_as_tuple(value),
            std::forward_as_tuple(program->pointer_type(program->int_type(8, false), false), value));
    }
    return &s_constant_strings.at(value);
}

Undef *Undef::get(const Type *type) {
    if (!s_undefs.contains(type)) {
        s_undefs.emplace(type, type);
    }
    return &s_undefs.at(type);
}

Constant *ConstantArray::clone(const Type *) const {
    ENSURE_NOT_REACHED();
}

Constant *ConstantInt::clone(const Type *type) const {
    return get(type, m_value);
}

Constant *ConstantNull::clone(const Type *type) const {
    return get(type);
}

Constant *ConstantString::clone(const Type *) const {
    ENSURE_NOT_REACHED();
}

Constant *Undef::clone(const Type *type) const {
    return get(type);
}

} // namespace ir
