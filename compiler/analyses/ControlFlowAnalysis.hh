#pragma once

#include <graph/DominatorTree.hh>
#include <graph/Graph.hh>
#include <ir/BasicBlock.hh>
#include <pass/Pass.hh>
#include <pass/PassResult.hh>

#include <unordered_map>
#include <unordered_set>
#include <vector>

struct ControlFlowAnalyser;

class ControlFlowAnalysis : public PassResult {
    friend ControlFlowAnalyser;

private:
    Graph<ir::BasicBlock> m_cfg;
    DominatorTree<ir::BasicBlock> m_dom_tree;
    std::unordered_map<ir::BasicBlock *, std::unordered_set<ir::BasicBlock *>> m_frontiers;

public:
    using analyser = ControlFlowAnalyser;

    explicit ControlFlowAnalysis(ir::BasicBlock *entry) : m_cfg(entry), m_dom_tree(entry) {}

    const std::vector<ir::BasicBlock *> &preds(ir::BasicBlock *block) const;
    const std::vector<ir::BasicBlock *> &succs(ir::BasicBlock *block) const;
    const std::vector<ir::BasicBlock *> &dominatees(ir::BasicBlock *block) const;
    const std::unordered_set<ir::BasicBlock *> &frontiers(ir::BasicBlock *block) const;
    ir::BasicBlock *entry() const;
};

struct ControlFlowAnalyser : public Pass {
    constexpr explicit ControlFlowAnalyser(PassManager *manager) : Pass(manager) {}

    void run(ir::Function *) override;
};
