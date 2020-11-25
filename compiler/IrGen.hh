#pragma once

#include <ast/Nodes.hh>
#include <ir/Program.hh>
#include <support/Box.hh>

#include <vector>

Box<ir::Program> gen_ir(std::vector<Box<ast::Root>> &&roots);
