#include <analyses/ReachingDefAnalysis.hh>

#include <analyses/ControlFlowAnalysis.hh>
#include <ir/Constants.hh>
#include <ir/Function.hh>
#include <ir/Instructions.hh>
#include <pass/PassManager.hh>
#include <pass/PassUsage.hh>
#include <support/Assert.hh>
#include <support/Stack.hh>

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

void ReachingDefAnalysis::put_reaching_def(ir::LoadInst *load, ir::Value *value) {
    ASSERT(!m_reaching_defs.contains(load));
    m_reaching_defs.emplace(load, value);
}

const List<MemoryPhi> &ReachingDefAnalysis::memory_phis(ir::BasicBlock *block) const {
    return m_memory_phis.at(block);
}

ir::Value *ReachingDefAnalysis::reaching_def(ir::LoadInst *load) const {
    return m_reaching_defs.at(load);
}

std::vector<ir::Value *> ReachingDefAnalysis::reaching_values(ir::LoadInst *load) const {
    std::vector<ir::Value *> values;
    auto *reaching = reaching_def(load);
    if (auto *phi = reaching != nullptr ? reaching->as_or_null<MemoryPhi>() : nullptr) {
        for (auto [block, value] : phi->incoming()) {
            values.push_back(value);
        }
    } else {
        values.push_back(reaching);
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
            auto *copy = inst->as_or_null<ir::CopyInst>();
            auto *store = inst->as_or_null<ir::StoreInst>();
            if (copy == nullptr && store == nullptr) {
                continue;
            }
            auto *ptr = copy != nullptr ? copy->dst() : store->ptr();
            for (auto *df : cfa->frontiers(block)) {
                if (visited_map[ptr].insert(df).second) {
                    memory_phis[df].emplace<MemoryPhi>(memory_phis[df].end(), ptr);
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
        for (auto *phi : memory_phis[block]) {
            def_stacks[phi->var()].push(phi);
        }

        for (auto *inst : *block) {
            if (auto *copy = inst->as_or_null<ir::CopyInst>()) {
                if (auto *constant = copy->len()->as_or_null<ir::Constant>()) {
                    std::size_t len = constant->as<ir::ConstantInt>()->value();
                    ASSERT(copy->src()->type()->size_in_bytes() == len);
                }
                def_stacks[copy->dst()].push(copy->src());
            } else if (auto *inline_asm = inst->as_or_null<ir::InlineAsmInst>()) {
                for (const auto &[output, output_val] : inline_asm->outputs()) {
                    def_stacks[output_val].push(inline_asm);
                }
            } else if (auto *load = inst->as_or_null<ir::LoadInst>()) {
                // TODO: pop_or_null helper function.
                auto &def_stack = def_stacks[load->ptr()];
                auto *reaching_def = !def_stack.empty() ? def_stack.peek() : ir::Undef::get(load->type());
                rda->put_reaching_def(load, reaching_def);
            } else if (auto *store = inst->as_or_null<ir::StoreInst>()) {
                def_stacks[store->ptr()].push(store->val());
            }
        }

        for (auto *succ : cfa->succs(block)) {
            for (auto *phi : memory_phis[succ]) {
                // TODO: pop_or_null helper function.
                auto &def_stack = def_stacks[phi->var()];
                auto *incoming = !def_stack.empty() ? def_stack.peek() : ir::Undef::get(phi->var()->type());
                phi->add_incoming(block, incoming);
            }
        }

        for (auto *succ : cfa->dominatees(block)) {
            work_queue.push(succ);
        }
    }
}
