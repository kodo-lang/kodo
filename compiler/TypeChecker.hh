#pragma once

#include <pass/Pass.hh>

struct TypeChecker : public Pass {
    constexpr explicit TypeChecker(PassManager *manager) : Pass(manager) {}

    void run(ir::Program *) override;
};
