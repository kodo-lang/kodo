#pragma once

#include <ir/Program.hh>

#include <memory>

class RootNode;

std::unique_ptr<Program> gen_ir(RootNode *ast);
