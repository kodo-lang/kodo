#include <TypeChecker.hh>

#include <ir/BasicBlock.hh>
#include <ir/Constants.hh>
#include <ir/Function.hh>
#include <ir/Instructions.hh>
#include <ir/Program.hh>
#include <ir/Types.hh>
#include <ir/Visitor.hh>
#include <support/Assert.hh>
#include <support/Error.hh>

namespace {

class Checker : public ir::Visitor {
    ir::Program *const m_program;
    ir::Function *m_function{nullptr};
    ir::BasicBlock *m_block{nullptr};
    ir::Instruction *m_instruction{nullptr};
    ir::BasicBlock::iterator m_insert_pos{nullptr};

    ir::Value *build_coerce_cast(ir::Value *value, const ir::Type *type, ir::CastOp op);
    ir::Value *coerce(ir::Value *value, const ir::Type *type);

public:
    explicit Checker(ir::Program *program) : m_program(program) {}

    void check(ir::Function *);
    void visit(ir::BinaryInst *) override;
    void visit(ir::BranchInst *) override;
    void visit(ir::CallInst *) override;
    void visit(ir::CastInst *) override;
    void visit(ir::CompareInst *) override;
    void visit(ir::CondBranchInst *) override;
    void visit(ir::CopyInst *) override;
    void visit(ir::InlineAsmInst *) override;
    void visit(ir::LeaInst *) override;
    void visit(ir::LoadInst *) override;
    void visit(ir::PhiInst *) override;
    void visit(ir::StoreInst *) override;
    void visit(ir::RetInst *) override;
};

const ir::Type *resulting_type(const ir::IntType *lhs, const ir::IntType *rhs) {
    return lhs->bit_width() > rhs->bit_width() ? lhs : rhs;
}

const ir::Type *resulting_type(const ir::IntType *lhs, const ir::Type *rhs) {
    switch (rhs->kind()) {
    case ir::TypeKind::Invalid:
        return lhs;
    case ir::TypeKind::Int:
        return resulting_type(lhs, rhs->as<ir::IntType>());
    case ir::TypeKind::Pointer:
        ENSURE_NOT_REACHED();
    default:
        ENSURE_NOT_REACHED();
    }
}

const ir::Type *resulting_type(const ir::Type *lhs, const ir::Type *rhs) {
    while (const auto *alias = lhs->as_or_null<ir::AliasType>()) {
        lhs = alias->aliased();
    }
    while (const auto *alias = rhs->as_or_null<ir::AliasType>()) {
        rhs = alias->aliased();
    }
    if (lhs->equals_weak(rhs)) {
        if (lhs->is<ir::InvalidType>()) {
            return lhs->cache()->int_type(32, true);
        }
        return lhs;
    }
    switch (lhs->kind()) {
    case ir::TypeKind::Int:
        return resulting_type(lhs->as<ir::IntType>(), rhs);
    case ir::TypeKind::Pointer:
        ENSURE_NOT_REACHED();
    default:
        ENSURE_NOT_REACHED();
    }
}

ir::Value *Checker::build_coerce_cast(ir::Value *value, const ir::Type *type, ir::CastOp op) {
    if (auto *constant = value->as_or_null<ir::Constant>()) {
        return constant->clone(type);
    }
    return m_block->insert<ir::CastInst>(m_insert_pos, op, type, value);
}

ir::Value *Checker::coerce(ir::Value *value, const ir::Type *rhs) {
    ASSERT(!rhs->is<ir::InvalidType>());
    const auto *lhs = value->type();
    if (lhs->equals_weak(rhs)) {
        return value;
    }
    if (lhs->is<ir::InvalidType>()) {
        return build_coerce_cast(value, rhs, ir::CastOp::SignExtend);
    }
    if (const auto *from = lhs->as_or_null<ir::IntType>()) {
        if (const auto *to = rhs->as_or_null<ir::IntType>()) {
            if (from->bit_width() < to->bit_width()) {
                return build_coerce_cast(value, rhs, ir::CastOp::SignExtend);
            }
        }
    }
    if (const auto *from = lhs->as_or_null<ir::PointerType>()) {
        if (const auto *to = rhs->as_or_null<ir::PointerType>()) {
            const auto *from_pointee = from->pointee_type();
            const auto *to_pointee = to->pointee_type();
            if (from_pointee->equals_weak(to_pointee) && from->is_mutable()) {
                ASSERT(!to->is_mutable());
                return value;
            }
            if (const auto *struct_type = ir::Type::base_as<ir::StructType>(from_pointee)) {
                if (const auto *trait_type = ir::Type::base_as<ir::TraitType>(to_pointee)) {
                    for (const auto *implementing : struct_type->implementing()) {
                        if (implementing->equals_weak(trait_type)) {
                            // TODO: Don't need this when not translating to LLVM.
                            return build_coerce_cast(value, rhs, ir::CastOp::Reinterpret);
                        }
                    }
                }
            }
        }
    }
    auto *inst = value->as_or_null<ir::Instruction>();
    if (inst == nullptr) {
        inst = m_instruction;
    }
    ENSURE(inst != nullptr);
    print_error(inst, "cannot implicitly cast from '{}' to '{}'", lhs->to_string(), rhs->to_string());
    return ir::ConstantNull::get(m_program->invalid_type());
}

void Checker::check(ir::Function *function) {
    m_function = function;
    for (auto *block : *function) {
        m_block = block;
        m_insert_pos = m_block->begin();
        for (auto *inst : *block) {
            m_instruction = inst;
            inst->accept(this);
            ++m_insert_pos;
        }
    }
}

void Checker::visit(ir::BinaryInst *binary) {
    auto *lhs = binary->lhs();
    auto *rhs = binary->rhs();
    const auto *type = resulting_type(lhs->type(), rhs->type());
    binary->set_type(type);
    binary->replace_uses_of_with(lhs, coerce(lhs, type));
    binary->replace_uses_of_with(rhs, coerce(rhs, type));
}

void Checker::visit(ir::BranchInst *) {}

void Checker::visit(ir::CallInst *call) {
    auto *callee = call->callee();
    const auto *function_type = callee->type()->as<ir::FunctionType>();
    const auto &args = call->args();
    const auto &params = function_type->params();
    if (args.size() != params.size()) {
        print_error(call, "'{}' requires {} arguments, but {} were passed", callee->name(), params.size(), args.size());
        return;
    }
    for (int i = 0; const auto *param : params) {
        auto *&arg = call->args()[i++];
        arg->remove_user(call);
        arg = coerce(arg, param);
        arg->add_user(call);
    }
}

void Checker::visit(ir::CastInst *cast) {
    auto *val = cast->val();
    if (const auto *from = val->type()->as_or_null<ir::IntType>()) {
        if (const auto *to = cast->type()->as_or_null<ir::IntType>()) {
            if (from->bit_width() <= to->bit_width()) {
                return;
            }
        }
    }
    if (val->type()->is<ir::BoolType>() && cast->type()->is<ir::IntType>()) {
        cast->set_op(ir::CastOp::ZeroExtend);
        return;
    }
    if (val->type()->is<ir::IntType>() && cast->type()->is<ir::PointerType>()) {
        cast->set_op(ir::CastOp::IntToPtr);
        return;
    }
    print_error(cast, "cannot cast from '{}' to '{}'", val->type()->to_string(), cast->type()->to_string());
}

void Checker::visit(ir::CompareInst *compare) {
    auto *lhs = compare->lhs();
    auto *rhs = compare->rhs();
    const auto *type = resulting_type(lhs->type(), rhs->type());
    lhs->replace_all_uses_with(coerce(lhs, type));
    rhs->replace_all_uses_with(coerce(rhs, type));
    compare->set_type(m_program->bool_type());
}

void Checker::visit(ir::CondBranchInst *cond_branch) {
    auto *cond = cond_branch->cond();
    cond_branch->replace_uses_of_with(cond, coerce(cond, m_program->bool_type()));
}

void Checker::visit(ir::CopyInst *) {}

void Checker::visit(ir::InlineAsmInst *) {}

void Checker::visit(ir::LeaInst *) {}

void Checker::visit(ir::LoadInst *) {}

void Checker::visit(ir::PhiInst *) {
    ASSERT_NOT_REACHED();
}

void Checker::visit(ir::StoreInst *store) {
    const auto *ptr_type = store->ptr()->type()->as<ir::PointerType>();
    const auto *type = resulting_type(ptr_type->pointee_type(), store->val()->type());
    store->replace_uses_of_with(store->val(), coerce(store->val(), type));
}

void Checker::visit(ir::RetInst *ret) {
    const auto *return_type = m_function->return_type();
    if (ret->val() == nullptr) {
        ASSERT(return_type->is<ir::VoidType>());
        return;
    }
    ret->replace_uses_of_with(ret->val(), coerce(ret->val(), return_type));
}

} // namespace

void TypeChecker::run(ir::Program *program) {
    Checker checker(program);
    for (auto *function : *program) {
        checker.check(function);
    }
}
