#include <VarChecker.hh>

#include <analyses/ReachingDefAnalysis.hh>
#include <ir/Constants.hh>
#include <ir/Function.hh>
#include <ir/Instructions.hh>
#include <ir/Types.hh>
#include <pass/PassUsage.hh>
#include <support/Error.hh>

void VarChecker::build_usage(PassUsage *usage) {
    usage->uses<ReachingDefAnalysis>();
}

void VarChecker::run(ir::Function *function) {
    if (function->begin() == function->end()) {
        return;
    }

    for (auto *var : function->vars()) {
        bool has_store = false;
        for (auto *user : var->users()) {
            auto *user_inst = user->as_or_null<ir::Instruction>();
            auto *store = user_inst != nullptr ? user_inst->as_or_null<ir::StoreInst>() : nullptr;
            bool is_assignment = store != nullptr && store->ptr() == var;
            if (is_assignment && has_store && !var->is_mutable()) {
                print_error(store, "attempted assignment of immutable variable '{}'", var->name());
            }
            has_store |= is_assignment;
        }
    }

    auto *rda = m_manager->get<ReachingDefAnalysis>(function);
    for (auto *block : *function) {
        for (auto *inst : *block) {
            if (auto *store = inst->as_or_null<ir::StoreInst>()) {
                // Ignore stores directly to a local variable's value, as they are handled above.
                if (store->ptr()->is<ir::LocalVar>()) {
                    continue;
                }
                const auto *type = store->ptr()->type()->as<ir::PointerType>();
                if (!type->is_mutable()) {
                    print_error(store, "attempted assignment of '{}' value pointed to by an immutable pointer",
                                type->pointee_type()->to_string());
                }
                continue;
            }

            auto *load = inst->as_or_null<ir::LoadInst>();
            if (load == nullptr) {
                continue;
            }
            auto *var = load->ptr()->as_or_null<ir::LocalVar>();
            if (var == nullptr) {
                continue;
            }
            // Ignore structs for now.
            // TODO: Work for structs.
            if (ir::Type::base(var->var_type())->is<ir::StructType>()) {
                continue;
            }
            for (auto *reaching_val : rda->reaching_values(load)) {
                auto *constant = reaching_val->as_or_null<ir::Constant>();
                if (constant != nullptr && constant->as_or_null<ir::Undef>() != nullptr) {
                    print_error(load, "use of possibly uninitialised variable '{}'", var->name());
                }
            }
        }
    }
}
