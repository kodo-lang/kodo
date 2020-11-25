#pragma once

#include <ast/Nodes.hh>
#include <ir/Program.hh>
#include <support/Box.hh>

#include <string>
#include <unordered_set>
#include <vector>

class Compiler {
    std::unordered_set<std::string> m_visited;
    std::vector<Box<ast::Root>> m_roots;

    void add_code(const std::string &path);

public:
    Box<ir::Program> compile(const std::string &main_path);
};
