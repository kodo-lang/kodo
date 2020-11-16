#include <ir/Function.hh>

#include <support/Assert.hh>

// TODO: List<T>::append()?
// TODO: Default List<T>::emplace() U param to T.
namespace ir {

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

} // namespace ir
