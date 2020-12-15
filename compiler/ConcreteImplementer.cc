#include <ConcreteImplementer.hh>

#include <ir/Constants.hh>
#include <ir/Function.hh>
#include <ir/Instructions.hh>
#include <ir/Program.hh>
#include <ir/Prototype.hh>
#include <ir/Types.hh>
#include <support/Error.hh>

#include <unordered_map>

namespace {

const ir::Type *concrete_type_of(ir::Value *value) {
    auto *inst = value->as_or_null<ir::Instruction>();
    auto *cast = inst != nullptr ? inst->as_or_null<ir::CastInst>() : nullptr;
    if (cast != nullptr) {
        value = cast->val();
    }
    return value->type()->as<ir::PointerType>()->pointee_type();
}

} // namespace

void ConcreteImplementer::run(ir::Program *program) {
    std::unordered_map<const ir::Prototype *, ir::Function *> implementers;
    for (auto *function : *program) {
        ENSURE(!implementers.contains(function->prototype()));
        implementers.emplace(function->prototype(), function);
        function->prototype()->replace_all_uses_with(function);
    }

    std::unordered_map<const ir::Type *, ir::GlobalVariable *> vtables;
    for (const auto &named_type : program->alias_types()) {
        const auto *struct_type = named_type->aliased()->as_or_null<ir::StructType>();
        if (struct_type == nullptr) {
            continue;
        }
        std::vector<ir::Value *> function_pointers;
        for (const auto *implementing : struct_type->implementing()) {
            const auto [name, trait] = ir::Type::expand_alias<ir::TraitType>(implementing);
            for (const auto *trait_prototype : trait->prototypes()) {
                auto it = std::find_if(struct_type->prototypes().begin(), struct_type->prototypes().end(),
                                       [trait_prototype](const auto *prototype) {
                                           return prototype->name() == trait_prototype->name();
                                       });
                if (it == struct_type->prototypes().end()) {
                    print_error("struct '{}' must implement '{}::{}'", named_type->name(), name,
                                trait_prototype->name());
                    continue;
                }
                ENSURE(implementers.contains(*it));
                function_pointers.push_back(implementers.at(*it));
            }
        }
        if (function_pointers.empty()) {
            continue;
        }
        auto *vtable = ir::ConstantArray::get(std::move(function_pointers));
        auto *vtable_global = program->append_global(vtable);
        vtable_global->set_name(fmt::format("{}.vtable", named_type->name()));
        vtables.emplace(*named_type, vtable_global);
    }
    for (auto *function : *program) {
        for (auto *arg : function->args()) {
            const auto *pointer_type = arg->type()->as_or_null<ir::PointerType>();
            const auto *trait_type =
                pointer_type != nullptr ? ir::Type::base_as<ir::TraitType>(pointer_type->pointee_type()) : nullptr;
            if (trait_type == nullptr) {
                continue;
            }
            const auto before_vptr_pos = std::distance(function->args().begin(), ListIterator<ir::Argument>(arg));
            const auto *vptr_type = program->pointer_type(program->pointer_type(program->void_type(), false), false);
            std::vector<const ir::Type *> params;
            for (int i = 0; i < function->prototype()->params().size(); i++) {
                params.push_back(function->prototype()->params()[i]);
                if (i == before_vptr_pos) {
                    params.push_back(vptr_type);
                }
            }
            const auto *new_function_type = program->function_type(function->return_type(), std::move(params));
            function->set_type(program->pointer_type(new_function_type, false));
            auto *vptr = function->insert_arg(arg, false);
            vptr->set_name(arg->name() + "_vptr");
            vptr->set_type(vptr_type);
            for (auto *block : *function) {
                // TODO: We wouldn't need to do this horrible iterator for a wrapping linked list (for future compiler).
                for (auto inst_it = block->begin(); inst_it != block->end();) {
                    auto *inst = *inst_it;
                    auto *call = inst->as_or_null<ir::CallInst>();
                    if (call == nullptr) {
                        ++inst_it;
                        continue;
                    }
                    const auto &prototypes = trait_type->prototypes();
                    auto it = std::find(prototypes.begin(), prototypes.end(), call->callee());
                    if (it == prototypes.end()) {
                        ++inst_it;
                        continue;
                    }
                    ++inst_it;
                    auto position = block->position(call);
                    std::vector<ir::Value *> indices;
                    indices.push_back(
                        ir::ConstantInt::get(program->int_type(64, false), std::distance(prototypes.begin(), it)));
                    auto *callee_ptr = block->insert<ir::LeaInst>(position, vptr, std::move(indices));
                    callee_ptr->set_type(program->pointer_type(program->pointer_type(it->type(), false), false));
                    auto *callee = block->insert<ir::LoadInst>(position, callee_ptr);
                    auto *new_call = block->insert<ir::CallInst>(position, callee, call->args());
                    call->replace_all_uses_with(new_call);
                    call->remove_from_parent();
                }
            }
            for (auto *user : function->users()) {
                auto *inst = user->as_or_null<ir::Instruction>();
                auto *call = inst != nullptr ? inst->as_or_null<ir::CallInst>() : nullptr;
                if (call == nullptr) {
                    continue;
                }
                auto *block = call->parent();
                auto position = block->position(call);
                std::vector<ir::Value *> args;
                for (int i = 0; i < call->args().size(); i++) {
                    auto *call_arg = call->args()[i];
                    args.push_back(call_arg);
                    if (i == before_vptr_pos) {
                        const auto *concrete_type = concrete_type_of(call_arg);
                        ir::Value *vtable = ir::Undef::get(vptr_type);
                        if (vtables.contains(concrete_type)) {
                            vtable = vtables.at(concrete_type);
                        }
                        auto *vtable_casted =
                            block->insert<ir::CastInst>(position, ir::CastOp::Reinterpret, vptr_type, vtable);
                        args.push_back(vtable_casted);
                    }
                }
                auto *new_call = block->insert<ir::CallInst>(position, call->callee(), std::move(args));
                call->replace_all_uses_with(new_call);
                call->remove_from_parent();
            }
        }
    }
}
