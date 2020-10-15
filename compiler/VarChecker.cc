#include <VarChecker.hh>

#include <Error.hh>
#include <analyses/ReachingDefAnalysis.hh>
#include <graph/Graph.hh>
#include <ir/Function.hh>
#include <ir/Instructions.hh>
#include <pass/PassManager.hh>
#include <pass/PassUsage.hh>
#include <support/Assert.hh>
#include <support/Stack.hh>

#include <string>
#include <unordered_map>
#include <vector>

void VarChecker::build_usage(PassUsage *usage) {
    usage->uses<ReachingDefAnalysis>();
}

void VarChecker::run(ir::Function *function) {
    if (function->begin() == function->end()) {
        return;
    }

    std::vector<std::string> errors;
    for (auto *var : function->vars()) {
        bool has_store = false;
        for (auto *user : var->users()) {
            auto *user_inst = user->as_or_null<ir::Instruction>();
            auto *store = user_inst != nullptr ? user_inst->as_or_null<ir::StoreInst>() : nullptr;
            bool is_assignment = store != nullptr && store->ptr() == var;
            if (is_assignment && has_store && !var->is_mutable()) {
                errors.push_back(
                    format_error(store, "attempted assignment of immutable variable '{}'", var->name()));
            }
            has_store |= is_assignment;
        }
    }

    auto *rda = m_manager->get<ReachingDefAnalysis>(function);
    for (auto *block : *function) {
        for (auto *inst : *block) {
            auto *load = inst->as_or_null<ir::LoadInst>();
            if (load == nullptr) {
                continue;
            }
            auto *var = load->ptr();
            if (var->kind() != ir::ValueKind::LocalVar) {
                continue;
            }
            for (auto *reaching_val : rda->reaching_values(load)) {
                if (reaching_val == nullptr) {
                    errors.push_back(format_error(load, "use of possibly uninitialised variable '{}'", var->name()));
                }
            }
        }
    }

    for (const auto &error : errors) {
        fmt::print(error);
    }
    if (!errors.empty()) {
        fmt::print(fmt::fg(fmt::color::orange_red), " note: Aborting due to previous errors\n");
        exit(1);
    }
}
