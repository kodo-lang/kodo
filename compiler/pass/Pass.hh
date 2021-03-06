#pragma once

namespace ir {

class Function;
class Program;

} // namespace ir

class PassManager;
class PassUsage;

class Pass {
protected:
    PassManager *const m_manager;

    constexpr explicit Pass(PassManager *manager) : m_manager(manager) {}

public:
    Pass(const Pass &) = delete;
    Pass(Pass &&) = delete;
    virtual ~Pass() = default;

    Pass &operator=(const Pass &) = delete;
    Pass &operator=(Pass &&) = delete;

    virtual void build_usage(PassUsage *) {}
    virtual void run(ir::Program *) {}
    virtual void run(ir::Function *) {}
};
