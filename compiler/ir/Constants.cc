#include <ir/Constants.hh>

#include <support/Assert.hh>

// TODO: Constant cache.
namespace ir {
namespace {

ConstantNull s_constant_null;

} // namespace

ConstantInt *ConstantInt::get(const Type *type, std::size_t value) {
    return new ConstantInt(type, value);
}

ConstantNull *ConstantNull::get() {
    return &s_constant_null;
}

ConstantString *ConstantString::get(std::string value) {
    return new ConstantString(PointerType::get(IntType::get_unsigned(8)), std::move(value));
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

} // namespace ir
