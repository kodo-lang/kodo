#pragma once

#include <pass/Pass.hh>

struct StackPromoter : public Pass {
    constexpr explicit StackPromoter(PassManager *manager) : Pass(manager) {}

    void build_usage(PassUsage *) override;
    void run(ir::Function *) override;
};
