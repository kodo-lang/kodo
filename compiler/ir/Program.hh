#pragma once

#include <ir/Function.hh>
#include <support/List.hh>

#include <string>

class Program {
    List<Function> m_functions;

public:
    Function *append_function(std::string name);
};
