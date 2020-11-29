#include <ir/Instruction.hh>

#include <ir/BasicBlock.hh>
#include <support/Assert.hh>

namespace ir {

ListIterator<Instruction> Instruction::remove_from_parent() {
    ASSERT(m_parent != nullptr);
    return m_parent->remove(this);
}

void Instruction::set_line(int line) {
    m_line = line;
}

} // namespace ir
