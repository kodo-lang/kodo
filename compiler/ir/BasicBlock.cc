#include <ir/BasicBlock.hh>

#include <support/Assert.hh>

#include <algorithm>

BasicBlock::iterator BasicBlock::position(const Instruction *inst) const {
    // TODO: No need to find!
    return std::find(m_instructions.begin(), m_instructions.end(), inst);
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

Instruction *BasicBlock::terminator() const {
    // TODO: Do we even need to check here?
    if (m_instructions.empty()) {
        return nullptr;
    }
    return *(--m_instructions.end());
}
