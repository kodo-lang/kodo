#pragma once

#include <ir/Program.hh>

#include <memory>

namespace ast {

class Root;

} // namespace ast

std::unique_ptr<ir::Program> gen_ir(const ast::Root *root);
