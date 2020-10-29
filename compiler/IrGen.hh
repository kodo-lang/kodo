#pragma once

#include <ast/Nodes.hh>
#include <ir/Program.hh>

#include <memory>
#include <vector>

std::unique_ptr<ir::Program> gen_ir(std::vector<std::unique_ptr<ast::Root>> &&roots);
