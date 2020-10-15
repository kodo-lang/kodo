#include <analyses/ControlFlowAnalysis.hh>

#include <graph/DominanceComputer.hh>
#include <ir/Function.hh>
#include <ir/Instructions.hh>
#include <pass/PassManager.hh>
#include <support/Assert.hh>

const std::vector<ir::BasicBlock *> &ControlFlowAnalysis::preds(ir::BasicBlock *block) const {
    return m_cfg.preds(block);
}

const std::vector<ir::BasicBlock *> &ControlFlowAnalysis::succs(ir::BasicBlock *block) const {
    return m_cfg.succs(block);
}

const std::vector<ir::BasicBlock *> &ControlFlowAnalysis::dominatees(ir::BasicBlock *block) const {
    return m_dom_tree.succs(block);
}

const std::unordered_set<ir::BasicBlock *> &ControlFlowAnalysis::frontiers(ir::BasicBlock *block) const {
    return const_cast<ControlFlowAnalysis *>(this)->m_frontiers[block];
}

ir::BasicBlock *ControlFlowAnalysis::entry() const {
    ASSERT(m_cfg.entry() == m_dom_tree.entry());
    return m_cfg.entry();
}

void ControlFlowAnalyser::run(ir::Function *function) {
    if (function->begin() == function->end()) {
        return;
    }

    auto *cfa = m_manager->make<ControlFlowAnalysis>(function, function->entry());
    auto &cfg = cfa->m_cfg;
    auto &dom_tree = cfa->m_dom_tree;
    auto &frontiers = cfa->m_frontiers;

    // Build CFG.
    for (auto *block : *function) {
        for (auto *inst : *block) {
            if (auto *branch = inst->as_or_null<ir::BranchInst>()) {
                cfg.connect(block, branch->dst());
            } else if (auto *cond_branch = inst->as_or_null<ir::CondBranchInst>()) {
                cfg.connect(block, cond_branch->true_dst());
                cfg.connect(block, cond_branch->false_dst());
            }
        }
    }

    // Build idom tree.
    // TODO: This would be nicer if the dominance computer modified the existing tree.
    auto tree = cfg.run<DominanceComputer>();
    dom_tree = std::move(tree);

    // Build dominance frontiers.
    // TODO: Better frontier algo.
    // TODO: Make this part of graph lib.
    auto tree_dfs = dom_tree.run<DepthFirstSearch>();
    for (auto *post_idom : tree_dfs.post_order()) {
        for (auto *succ : cfg.succs(post_idom)) {
            if (dom_tree.idom(succ) != post_idom) {
                frontiers[post_idom].insert(succ);
            }
        }
        for (auto *f : dom_tree.succs(post_idom)) {
            for (auto *ff : frontiers[f]) {
                if (dom_tree.idom(ff) != post_idom) {
                    frontiers[post_idom].insert(ff);
                }
            }
        }
    }
}
