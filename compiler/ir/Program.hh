#pragma once

#include <ir/Function.hh>
#include <ir/GlobalVariable.hh>
#include <ir/TypeCache.hh>
#include <support/List.hh>

#include <string>
#include <vector>

namespace ir {

class Program : public TypeCache {
    List<Function> m_functions;
    List<GlobalVariable> m_globals;
    List<Prototype> m_prototypes;
    std::vector<Box<Type>> m_types;

public:
    using iterator = decltype(m_functions)::iterator;
    iterator begin() const { return m_functions.begin(); }
    iterator end() const { return m_functions.end(); }

    template <typename... Args>
    Function *append_function(Args &&... args) {
        return m_functions.emplace<Function>(m_functions.end(), std::forward<Args>(args)...);
    }

    template <typename... Args>
    GlobalVariable *append_global(Args &&... args) {
        return m_globals.emplace<GlobalVariable>(m_globals.end(), std::forward<Args>(args)...);
    }

    void append_prototype(Prototype *prototype) { return m_prototypes.insert(m_prototypes.end(), prototype); }

    template <typename Ty, typename... Args>
    Ty *make(Args &&... args) requires std::derived_from<Ty, Type> {
        return static_cast<Ty *>(*m_types.emplace_back(new Ty(this, std::forward<Args>(args)...)));
    }

    const List<GlobalVariable> &globals() const { return m_globals; }
    const List<Prototype> &prototypes() const { return m_prototypes; }
};

} // namespace ir
