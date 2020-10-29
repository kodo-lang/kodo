#pragma once

#include <ast/Nodes.hh>
#include <ir/Program.hh>

#include <memory>
#include <string>
#include <unordered_set>
#include <vector>

class Compiler {
    std::unordered_set<std::string> m_visited;
    std::vector<std::unique_ptr<ast::Root>> m_roots;

    void add_code(const std::string &path);

public:
    std::unique_ptr<ir::Program> compile(const std::string &main_path);
};
