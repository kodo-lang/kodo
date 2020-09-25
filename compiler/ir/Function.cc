#include <ir/Function.hh>

#include <cassert>

// TODO: List<T>::append()?
// TODO: Default List<T>::emplace() U param to T.

Argument *Function::append_arg() {
    return m_args.emplace<Argument>(m_args.end());
}

BasicBlock *Function::append_block() {
    auto *block = m_blocks.emplace<BasicBlock>(m_blocks.end());
    block->set_parent(this);
    return block;
}

LocalVar *Function::append_var(Type *type) {
    return m_vars.emplace<LocalVar>(m_vars.end(), type);
}

BasicBlock *Function::entry() const {
    assert(!m_blocks.empty());
    return *m_blocks.begin();
}
