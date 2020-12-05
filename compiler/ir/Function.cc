#include <ir/Function.hh>

#include <ir/TypeCache.hh>
#include <ir/Types.hh>
#include <support/Assert.hh>

#include <utility>

// TODO: List<T>::append()?
// TODO: Default List<T>::emplace() U param to T.
namespace ir {

LocalVar::LocalVar(const Type *var_type, bool is_mutable) : Value(KIND), m_var_type(var_type) {
    set_type(m_var_type->cache()->pointer_type(m_var_type, is_mutable));
}

void LocalVar::set_var_type(const Type *var_type) {
    m_var_type = var_type;
    set_type(m_var_type->cache()->pointer_type(m_var_type, is_mutable()));
}

bool LocalVar::is_mutable() const {
    return type()->as<PointerType>()->is_mutable();
}

Function::Function(const Prototype *prototype, std::string mangled_name, const FunctionType *type)
    : Value(KIND), m_prototype(prototype) {
    set_name(std::move(mangled_name));
    set_type(type);
}

Argument *Function::append_arg(bool is_mutable) {
    return m_args.emplace<Argument>(m_args.end(), is_mutable);
}

BasicBlock *Function::append_block() {
    auto *block = m_blocks.emplace<BasicBlock>(m_blocks.end());
    block->set_parent(this);
    return block;
}

LocalVar *Function::append_var(const Type *type, bool is_mutable) {
    return m_vars.emplace<LocalVar>(m_vars.end(), type, is_mutable);
}

void Function::remove_var(LocalVar *var) {
    ASSERT(var->users().empty());
    m_vars.erase(ListIterator<LocalVar>(var));
}

BasicBlock *Function::entry() const {
    ASSERT(!m_blocks.empty());
    return *m_blocks.begin();
}

const Type *Function::return_type() const {
    return type()->as<FunctionType>()->return_type();
}

} // namespace ir
