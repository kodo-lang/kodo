#include <pass/PassManager.hh>

#include <ir/Program.hh>
#include <pass/PassUsage.hh>

#include <unordered_map>

void PassManager::run_pass(ir::Program *program, Pass *pass, std::unordered_map<Pass *, bool> &ready_map) {
    PassUsage usage(this);
    pass->build_usage(&usage);
    for (auto *dependency : usage.m_dependencies) {
        if (!ready_map[dependency]) {
            run_pass(program, dependency, ready_map);
        }
    }
    pass->run(program);
    for (auto *function : *program) {
        pass->run(function);
    }
    ready_map[pass] = true;
}

void PassManager::run(ir::Program *program) {
    std::unordered_map<Pass *, bool> ready_map;
    for (auto *transform : m_transforms) {
        run_pass(program, transform, ready_map);
    }
}
