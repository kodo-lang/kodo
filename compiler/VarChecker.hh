#pragma once

#include <pass/Pass.hh>

struct VarChecker : public Pass {
    constexpr explicit VarChecker(PassManager *manager) : Pass(manager) {}

    void run(ir::Function *) override;
};
