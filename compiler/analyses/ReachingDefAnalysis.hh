#pragma once

#include <ir/Value.hh>
#include <pass/Pass.hh>
#include <pass/PassResult.hh>
#include <support/List.hh>
#include <support/ListNode.hh>

#include <unordered_map>
#include <vector>

namespace ir {

class BasicBlock;
class LoadInst;

} // namespace ir

struct ReachingDefAnalyser;

class MemoryPhi : public ir::Value, public ListNode {
    ir::Value *const m_var;
    std::unordered_map<ir::BasicBlock *, ir::Value *> m_incoming;

public:
    static constexpr auto KIND = ir::ValueKind::MemoryPhi;

    explicit MemoryPhi(ir::Value *var) : ir::Value(KIND), m_var(var) {}
    ~MemoryPhi() override;

    void add_incoming(ir::BasicBlock *block, ir::Value *value);
    void replace_uses_of_with(Value *orig, Value *repl) override;

    ir::Value *var() const { return m_var; }
    const std::unordered_map<ir::BasicBlock *, ir::Value *> &incoming() const { return m_incoming; }
};

class ReachingDefAnalysis : public PassResult {
    friend ReachingDefAnalyser;

private:
    std::unordered_map<ir::BasicBlock *, List<MemoryPhi>> m_memory_phis;
    std::unordered_map<ir::LoadInst *, ir::Value *> m_reaching_defs;

    void put_reaching_def(ir::LoadInst *, ir::Value *);

public:
    using analyser = ReachingDefAnalyser;

    const List<MemoryPhi> &memory_phis(ir::BasicBlock *block) const;
    ir::Value *reaching_def(ir::LoadInst *load) const;
    std::vector<ir::Value *> reaching_values(ir::LoadInst *load) const;
};

struct ReachingDefAnalyser : public Pass {
    constexpr explicit ReachingDefAnalyser(PassManager *manager) : Pass(manager) {}

    void build_usage(PassUsage *) override;
    void run(ir::Function *) override;
};
