#include <StackPromoter.hh>

#include <analyses/ReachingDefAnalysis.hh>
#include <ir/Function.hh>
#include <ir/Instructions.hh>
#include <pass/PassUsage.hh>

#include <unordered_map>
#include <unordered_set>

namespace {

bool is_promotable(ir::LocalVar *var) {
    for (auto *user : var->users()) {
        auto *inst = user->as_or_null<ir::Instruction>();
        if (inst == nullptr) {
            continue;
        }
        if (inst->as_or_null<ir::CallInst>() != nullptr) {
            return false;
        }
        if (inst->as_or_null<ir::CastInst>() != nullptr) {
            return false;
        }
        // TODO: Better inline asm output handling.
        if (inst->as_or_null<ir::InlineAsmInst>() != nullptr) {
            return false;
        }
        if (inst->as_or_null<ir::LeaInst>() != nullptr) {
            return false;
        }
        auto *store = inst->as_or_null<ir::StoreInst>();
        if (store != nullptr && store->ptr() != var) {
            return false;
        }
    }
    return true;
}

} // namespace

void StackPromoter::build_usage(PassUsage *usage) {
    usage->uses<ReachingDefAnalysis>();
}

void StackPromoter::run(ir::Function *function) {
    if (function->begin() == function->end()) {
        return;
    }

    std::unordered_set<ir::LocalVar *> promotable_vars;
    for (auto *var : function->vars()) {
        if (is_promotable(var)) {
            promotable_vars.insert(var);
        }
    }

    auto *rda = m_manager->get<ReachingDefAnalysis>(function);
    std::unordered_map<MemoryPhi *, ir::PhiInst *> phi_map;
    for (auto *block : *function) {
        for (auto *memory_phi : rda->memory_phis(block)) {
            auto *var = memory_phi->var()->as_or_null<ir::LocalVar>();
            if (var == nullptr || !promotable_vars.contains(var)) {
                continue;
            }
            ASSERT(!phi_map.contains(memory_phi));
            auto *phi = block->prepend<ir::PhiInst>();
            phi_map.emplace(memory_phi, phi);
            for (auto [block, value] : memory_phi->incoming()) {
                if (auto *incoming_memory_phi = value != nullptr ? value->as_or_null<MemoryPhi>() : nullptr) {
                    value = phi_map.at(incoming_memory_phi);
                }
                phi->add_incoming(block, value);
                if (value != nullptr) {
                    phi->set_type(value->type());
                }
            }
        }
    }

    for (auto *var : promotable_vars) {
        for (auto *user : std::vector(var->users())) {
            auto *inst = user->as_or_null<ir::Instruction>();
            if (inst == nullptr) {
                continue;
            }
            if (auto *load = inst->as_or_null<ir::LoadInst>()) {
                auto *reaching = rda->reaching_def(load);
                if (auto *memory_phi = reaching != nullptr ? reaching->as_or_null<MemoryPhi>() : nullptr) {
                    reaching = phi_map.at(memory_phi);
                }
                load->replace_all_uses_with(reaching);
                load->remove_from_parent();
            } else if (auto *store = inst->as_or_null<ir::StoreInst>()) {
                store->remove_from_parent();
            }
        }
        function->remove_var(var);
    }
}
