#include <ir/Prototype.hh>

#include <ir/Types.hh>

#include <utility>

namespace ir {

Prototype::Prototype(bool externed, std::string name, const FunctionType *type) : Value(KIND), m_externed(externed) {
    set_name(std::move(name));
    set_type(type);
}

const Type *Prototype::return_type() const {
    return type()->as<FunctionType>()->return_type();
}

} // namespace ir
