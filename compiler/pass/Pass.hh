#pragma once

namespace ir {

class Function;
class Program;

} // namespace ir

class Pass {
protected:
    Pass() = default;

public:
    Pass(const Pass &) = delete;
    Pass(Pass &&) = delete;
    virtual ~Pass() = default;

    Pass &operator=(const Pass &) = delete;
    Pass &operator=(Pass &&) = delete;

    virtual void run(ir::Program *) {}
    virtual void run(ir::Function *) {}
};
