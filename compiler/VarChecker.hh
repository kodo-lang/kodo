#pragma once

#include <pass/Pass.hh>

struct VarChecker : public Pass {
    void run(ir::Function *) override;
};
