#pragma once

#include <pass/Pass.hh>

namespace ir {

class Program;

} // namespace ir

struct TypeChecker : public Pass {
    void run(ir::Program *) override;
};
