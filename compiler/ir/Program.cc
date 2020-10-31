#include <ir/Program.hh>

#include <utility>

Function *Program::append_function(std::string name) {
    // TODO: List<T>::append()?
    // TODO: Default List<T>::emplace() U param to T.
    return m_functions.emplace<Function>(m_functions.end(), std::move(name));
}
