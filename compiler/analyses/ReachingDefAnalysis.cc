#include <analyses/ReachingDefAnalysis.hh>

#include <analyses/ControlFlowAnalysis.hh>
#include <ir/Function.hh>
#include <ir/Instructions.hh>
#include <pass/PassManager.hh>
#include <pass/PassUsage.hh>
#include <support/Assert.hh>
#include <support/Stack.hh>

#include <ir/Constants.hh>

#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

MemoryPhi::~MemoryPhi() {
    for (auto [block, value] : m_incoming) {
        ASSERT(block != nullptr);
        block->remove_user(this);
        if (value != nullptr) {
            value->remove_user(this);
        }
    }
}

void MemoryPhi::add_incoming(ir::BasicBlock *block, ir::Value *value) {
    // TODO: Do we need to add users here?
    ASSERT(block != nullptr);
    block->add_user(this);
    if (value != nullptr) {
        value->add_user(this);
    }
    m_incoming[block] = value;
}

void MemoryPhi::replace_uses_of_with(Value *orig, Value *repl) {
    ASSERT(repl == nullptr);
    for (auto [block, value] : m_incoming) {
        ASSERT(block != orig);
        if (value == orig) {
            ASSERT(value != nullptr);
            value->remove_user(this);
            m_incoming[block] = repl;
        }
    }
}

void ReachingDefAnalysis::add_reaching_def(ir::LoadInst *load, ir::Value *value) {
    m_reaching_defs[load].push_back(value);
}

const std::vector<ir::Value *> &ReachingDefAnalysis::reaching_defs(ir::LoadInst *load) const {
    return m_reaching_defs.at(load);
}

std::vector<ir::Value *> ReachingDefAnalysis::reaching_values(ir::LoadInst *load) const {
    std::vector<ir::Value *> values;
    for (auto *reaching_def : reaching_defs(load)) {
        auto *phi = reaching_def != nullptr ? reaching_def->as_or_null<MemoryPhi>() : nullptr;
        if (phi == nullptr) {
            values.push_back(reaching_def);
            continue;
        }
        for (auto [block, value] : phi->incoming()) {
            values.push_back(value);
        }
    }
    return std::move(values);
}

void ReachingDefAnalyser::build_usage(PassUsage *usage) {
    usage->uses<ControlFlowAnalysis>();
}

void ReachingDefAnalyser::run(ir::Function *function) {
    if (function->begin() == function->end()) {
        return;
    }

    auto *cfa = m_manager->get<ControlFlowAnalysis>(function);
    auto *rda = m_manager->make<ReachingDefAnalysis>(function);
    auto &memory_phis = rda->m_memory_phis;
    std::unordered_map<ir::Value *, std::unordered_set<ir::BasicBlock *>> visited_map;
    for (auto *block : *function) {
        for (auto *inst : *block) {
            auto *store = inst->as_or_null<ir::StoreInst>();
            if (store == nullptr) {
                continue;
            }
            for (auto *df : cfa->frontiers(block)) {
                if (visited_map[store->ptr()].insert(df).second) {
                    memory_phis[df].emplace_back(new MemoryPhi(store->ptr()));
                }
            }
        }
    }

    // TODO: Iterable Queue class.
    Stack<ir::BasicBlock *> work_queue;
    work_queue.push(cfa->entry());
    std::unordered_map<ir::Value *, Stack<ir::Value *>> def_stacks;
    while (!work_queue.empty()) {
        auto *block = work_queue.pop();
        for (auto &phi : memory_phis[block]) {
            def_stacks[phi->var()].push(phi.get());
        }

        for (auto *inst : *block) {
            if (auto *load = inst->as_or_null<ir::LoadInst>()) {
                // TODO: pop_or_null helper function.
                auto &def_stack = def_stacks[load->ptr()];
                auto *reaching_def = !def_stack.empty() ? def_stack.peek() : nullptr;
                rda->add_reaching_def(load, reaching_def);
            } else if (auto *store = inst->as_or_null<ir::StoreInst>()) {
                def_stacks[store->ptr()].push(store->val());
            }
        }

        for (auto *succ : cfa->succs(block)) {
            for (auto &phi : memory_phis[succ]) {
                // TODO: pop_or_null helper function.
                auto &def_stack = def_stacks[phi->var()];
                auto *incoming = !def_stack.empty() ? def_stack.peek() : nullptr;
                phi->add_incoming(block, incoming);
            }
        }

        for (auto *succ : cfa->dominatees(block)) {
            work_queue.push(succ);
        }
    }
}
