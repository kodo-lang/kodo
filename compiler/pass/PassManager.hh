#pragma once

#include <pass/Pass.hh>
#include <pass/PassResult.hh>
#include <support/Assert.hh>

#include <concepts>
#include <memory>
#include <typeindex>
#include <unordered_map>
#include <utility>
#include <vector>

class PassUsage;

class PassManager {
    friend PassUsage;

private:
    std::unordered_map<std::type_index, std::unique_ptr<Pass>> m_pass_map;
    std::unordered_map<const void *, std::unordered_map<std::type_index, std::unique_ptr<PassResult>>> m_results;
    std::vector<Pass *> m_transforms;

    template <typename T, typename... Args>
    Pass *ensure_pass(Args &&... args);

    void run_pass(ir::Program *program, Pass *pass, std::unordered_map<Pass *, bool> &ready_map);

public:
    template <typename T, typename... Args>
    void add(Args &&... args) requires std::derived_from<T, Pass>;

    template <typename T, typename... Args>
    T *make(const void *obj, Args &&... args);

    template <typename T>
    T *get(const void *obj);

    void run(ir::Program &program);
};

template <typename T, typename... Args>
Pass *PassManager::ensure_pass(Args &&... args) {
    std::type_index type(typeid(T));
    if (!m_pass_map.contains(type)) {
        m_pass_map.emplace(type, new T(this, std::forward<Args>(args)...));
    }
    return m_pass_map.at(type).get();
}

template <typename T, typename... Args>
void PassManager::add(Args &&... args) requires std::derived_from<T, Pass> {
    m_transforms.push_back(ensure_pass<T>(std::forward<Args>(args)...));
}

template <typename T, typename... Args>
T *PassManager::make(const void *obj, Args &&... args) {
    auto ptr = std::make_unique<T>(std::forward<Args>(args)...);
    auto *ret = ptr.get();
    m_results[obj][std::type_index(typeid(T))] = std::move(ptr);
    return ret;
}

template <typename T>
T *PassManager::get(const void *obj) {
    auto *ptr = m_results.at(obj).at(std::type_index(typeid(T))).get();
    ASSERT_PEDANTIC(dynamic_cast<T *>(ptr) != nullptr);
    return static_cast<T *>(ptr);
}
