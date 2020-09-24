#pragma once

#include <ir/Program.hh>

#include <memory>

class AstNode;

std::unique_ptr<Program> gen_ir(AstNode *ast);
