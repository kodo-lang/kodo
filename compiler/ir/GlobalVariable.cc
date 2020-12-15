#include <ir/GlobalVariable.hh>

#include <support/Assert.hh>

namespace ir {

GlobalVariable::GlobalVariable(const ir::Constant *initialiser) : Value(KIND), m_initialiser(initialiser) {
    ASSERT(m_initialiser != nullptr);
    set_type(initialiser->type());
}

} // namespace ir
