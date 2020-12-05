#pragma once

#include <pass/Pass.hh>

namespace ir {

struct Dumper : public Pass {
    constexpr explicit Dumper(PassManager *manager) : Pass(manager) {}

    void run(Program *) override;
    void run(Function *) override;
};

} // namespace ir
