#pragma once

#include <pass/Pass.hh>

#include <concepts>
#include <memory>
#include <utility>
#include <vector>

class PassManager {
    std::vector<std::unique_ptr<Pass>> m_passes;

public:
    template <typename T, typename... Args>
    void add(Args &&... args) requires std::derived_from<T, Pass>;
    void run(ir::Program &program);
};

template <typename T, typename... Args>
void PassManager::add(Args &&... args) requires std::derived_from<T, Pass> {
    m_passes.emplace_back(new T(std::forward<Args>(args)...));
}
