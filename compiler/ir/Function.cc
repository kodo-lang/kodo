#include <ir/Function.hh>

#include <cassert>

BasicBlock *Function::append_block() {
    // TODO: List<T>::append()?
    // TODO: Default List<T>::emplace() U param to T.
    auto *block = m_blocks.emplace<BasicBlock>(m_blocks.end());
    block->set_parent(this);
    return block;
}

BasicBlock *Function::entry() const {
    assert(!m_blocks.empty());
    return *m_blocks.begin();
}
