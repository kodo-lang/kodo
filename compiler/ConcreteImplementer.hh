#pragma once

#include <pass/Pass.hh>

struct ConcreteImplementer : public Pass {
    constexpr explicit ConcreteImplementer(PassManager *manager) : Pass(manager) {}

    void run(ir::Program *) override;
};
