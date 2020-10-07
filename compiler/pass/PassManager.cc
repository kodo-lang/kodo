#include <pass/PassManager.hh>

#include <ir/Program.hh>

void PassManager::run(ir::Program &program) {
    for (const auto &pass : m_passes) {
        pass->run(&program);
        for (auto *function : program) {
            pass->run(function);
        }
    }
}
