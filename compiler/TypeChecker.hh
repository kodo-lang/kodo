#pragma once

#include <pass/Pass.hh>

struct TypeChecker : public Pass {
    void run(ir::Program *) override;
};
