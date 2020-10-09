#include <VarChecker.hh>

#include <Error.hh>
#include <ir/Function.hh>
#include <ir/Instructions.hh>
#include <support/Assert.hh>
#include <support/Stack.hh>

#include <string>
#include <unordered_map>
#include <vector>

void VarChecker::run(ir::Function *function) {
    std::unordered_map<ir::LocalVar *, Stack<ir::Instruction *>> stack_map;
    std::unordered_map<ir::Instruction *, std::vector<ir::LocalVar *>> user_map;
    std::vector<std::string> errors;
    for (auto *var : function->vars()) {
        bool alive = false;
        for (auto *user : var->users()) {
            if (auto *user_inst = user->as_or_null<ir::Instruction>()) {
                user_map[user_inst].push_back(var);
                alive |= user_inst->inst_kind() == ir::InstKind::Load;
                if (auto *store = user_inst->as_or_null<ir::StoreInst>()) {
                    alive |= store->val() == var;
                }
            }
        }
        if (!alive) {
            // TODO: The line number isn't right.
            auto *inst = *function->entry()->begin();
            fmt::print(format_warn(inst, "variable '{}' is unused", var->name()));
        }
    }
    for (auto *block : *function) {
        for (auto *inst : *block) {
            if (!user_map.contains(inst)) {
                continue;
            }
            for (auto *var : user_map.at(inst)) {
                auto &stack = stack_map[var];
                if (auto *load = inst->as_or_null<ir::LoadInst>()) {
                    if (stack.empty()) {
                        errors.push_back(format_error(load, "use of variable '{}' before initialization", var->name()));
                    }
                } else if (auto *store = inst->as_or_null<ir::StoreInst>()) {
                    if (store->val() != var) {
                        if (!stack.empty() && !var->is_mutable()) {
                            errors.push_back(
                                format_error(store, "attempted assignment of immutable variable '{}'", var->name()));
                        }
                        stack.push(store);
                    }
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
