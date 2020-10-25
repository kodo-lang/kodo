#include <ast/Type.hh>

namespace ast {

Type Type::get_base(std::string &&base) {
    return {TypeKind::Base, std::move(base)};
}

Type Type::get_pointer(Type &&pointee, bool is_mutable) {
    return {TypeKind::Pointer, std::make_pair(std::make_unique<Type>(std::move(pointee)), is_mutable)};
}

Type Type::get_struct(std::vector<StructField> &&fields) {
    return {TypeKind::Struct, std::move(fields)};
}

} // namespace ast
