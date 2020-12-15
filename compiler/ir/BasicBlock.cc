#include <ir/BasicBlock.hh>

#include <support/Assert.hh>

#include <algorithm>

namespace ir {

BasicBlock::iterator BasicBlock::position(Instruction *inst) const {
    return BasicBlock::iterator(inst);
}

BasicBlock::iterator BasicBlock::remove(Instruction *inst) {
    ASSERT(inst->users().empty());
    auto it = position(inst);
    ASSERT(it != m_instructions.end());
    return m_instructions.erase(it);
}

bool BasicBlock::has_parent() const {
    return m_parent != nullptr;
}

void BasicBlock::set_parent(Function *parent) {
    m_parent = parent;
}

bool BasicBlock::empty() const {
    return m_instructions.empty();
}

Instruction *BasicBlock::terminator() const {
    ASSERT(!m_instructions.empty());
    return *(--m_instructions.end());
}

} // namespace ir
