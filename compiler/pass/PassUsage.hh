#pragma once

#include <pass/Pass.hh>
#include <pass/PassManager.hh>

#include <vector>

template <typename T>
concept HasAnalyser = requires(T *t) {
    typename T::analyser;
};

class PassUsage {
    friend PassManager;

private:
    PassManager *const m_manager;
    std::vector<Pass *> m_dependencies;

    explicit PassUsage(PassManager *manager) : m_manager(manager) {}

public:
    template <typename T>
    void uses() requires HasAnalyser<T>;
};

template <typename T>
void PassUsage::uses() requires HasAnalyser<T> {
    auto *pass = m_manager->ensure_pass<typename T::analyser>();
    m_dependencies.push_back(pass);
}
