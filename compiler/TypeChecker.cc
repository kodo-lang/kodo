#include <TypeChecker.hh>

#include <ir/BasicBlock.hh>
#include <ir/Constants.hh>
#include <ir/Function.hh>
#include <ir/Instructions.hh>
#include <ir/Program.hh>
#include <ir/Visitor.hh>
#include <support/Assert.hh>
#include <support/Error.hh>

namespace {

class Checker : public ir::Visitor {
    ir::Function *m_function{nullptr};
    ir::BasicBlock *m_block{nullptr};
    ir::Instruction *m_instruction{nullptr};
    ir::BasicBlock::iterator m_insert_pos{nullptr};

    ir::Value *build_coerce_cast(ir::Value *value, const ir::Type *type, ir::CastOp op);
    ir::Value *coerce(ir::Value *value, const ir::Type *type);

public:
    void check(ir::Function *);
    void visit(ir::BinaryInst *) override;
    void visit(ir::BranchInst *) override;
    void visit(ir::CallInst *) override;
    void visit(ir::CastInst *) override;
    void visit(ir::CompareInst *) override;
    void visit(ir::CondBranchInst *) override;
    void visit(ir::CopyInst *) override;
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
    if (lhs == rhs) {
        // TODO: Hacky. resulting_type should take in two values and have special handling for constants.
        if (lhs->is<ir::InvalidType>()) {
            return ir::IntType::get_signed(32);
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

ir::Value *Checker::coerce(ir::Value *value, const ir::Type *type) {
    ASSERT(!type->is<ir::InvalidType>());
    if (value->type() == type) {
        return value;
    }
    if (value->type()->is<ir::InvalidType>()) {
        return build_coerce_cast(value, type, ir::CastOp::SignExtend);
    }
    if (const auto *from = value->type()->as_or_null<ir::IntType>()) {
        if (const auto *to = type->as_or_null<ir::IntType>()) {
            if (from->bit_width() < to->bit_width()) {
                return build_coerce_cast(value, type, ir::CastOp::SignExtend);
            }
        }
    }
    if (const auto *from = value->type()->as_or_null<ir::PointerType>()) {
        if (const auto *to = type->as_or_null<ir::PointerType>()) {
            if (from->pointee_type() == to->pointee_type() && from->is_mutable()) {
                ASSERT(!to->is_mutable());
                return value;
            }
        }
    }
    auto *inst = value->as_or_null<ir::Instruction>();
    if (inst == nullptr) {
        inst = m_instruction;
    }
    ENSURE(inst != nullptr);
    print_error(inst, "cannot implicitly cast from '{}' to '{}'", value->type()->to_string(), type->to_string());
    return ir::ConstantNull::get();
}

void Checker::check(ir::Function *function) {
    m_function = function;
    ASSERT(function->return_type() != nullptr);
    for (auto *arg : function->args()) {
        ASSERT(arg->has_type());
    }
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
    if (call->args().size() != callee->args().size()) {
        print_error(call, "'{}' requires {} arguments, but {} were passed", callee->name(), callee->args().size(),
                  call->args().size());
        return;
    }
    for (int i = 0; auto *param : callee->args()) {
        auto *arg = call->args().at(i++);
        call->replace_uses_of_with(arg, coerce(arg, param->type()));
    }
    call->set_type(callee->return_type());
}

void Checker::visit(ir::CastInst *cast) {
    auto *val = cast->val();
    ENSURE(val->kind() != ir::ValueKind::Constant);
    if (val->type()->is<ir::BoolType>() && cast->type()->is<ir::IntType>()) {
        cast->set_op(ir::CastOp::ZeroExtend);
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
    compare->set_type(ir::BoolType::get());
}

void Checker::visit(ir::CondBranchInst *cond_branch) {
    auto *cond = cond_branch->cond();
    cond_branch->replace_uses_of_with(cond, coerce(cond, ir::BoolType::get()));
}

void Checker::visit(ir::CopyInst *) {}

void Checker::visit(ir::LeaInst *) {}

void Checker::visit(ir::LoadInst *load) {
    const auto *ptr_type = load->ptr()->type()->as<ir::PointerType>();
    load->set_type(ptr_type->pointee_type());
}

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
    Checker checker;
    for (auto *function : *program) {
        checker.check(function);
    }
}
