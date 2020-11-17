#pragma once

#include <ir/Function.hh>
#include <ir/TypeCache.hh>
#include <support/List.hh>

#include <string>

namespace ir {

class Program : public TypeCache {
    List<Function> m_functions;

public:
    using iterator = decltype(m_functions)::iterator;
    iterator begin() const { return m_functions.begin(); }
    iterator end() const { return m_functions.end(); }

    Function *append_function(std::string name, const Type *return_type);
};

} // namespace ir
