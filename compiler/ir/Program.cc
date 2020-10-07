#include <ir/Program.hh>

#include <utility>

namespace ir {

Function *Program::append_function(std::string name, const Type *return_type) {
    // TODO: List<T>::append()?
    // TODO: Default List<T>::emplace() U param to T.
    return m_functions.emplace<Function>(m_functions.end(), std::move(name), return_type);
}

} // namespace ir
