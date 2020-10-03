#include <ir/Instruction.hh>

#include <ir/BasicBlock.hh>
#include <support/Assert.hh>

ListIterator<Instruction> Instruction::remove_from_parent() {
    ASSERT(has_parent());
    return m_parent->remove(this);
}

bool Instruction::has_parent() const {
    return m_parent != nullptr;
}

void Instruction::set_line(int line) {
    m_line = line;
}

void Instruction::set_parent(BasicBlock *parent) {
    m_parent = parent;
}
