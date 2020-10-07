#pragma once

#include <pass/Pass.hh>

namespace ir {

struct Dumper : public Pass {
    void run(Function *) override;
};

} // namespace ir
