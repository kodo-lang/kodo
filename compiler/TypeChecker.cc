#include <TypeChecker.hh>

#include <ir/BasicBlock.hh>
#include <ir/Constant.hh>
#include <ir/Function.hh>
#include <ir/Instructions.hh>
#include <ir/Program.hh>
#include <ir/Visitor.hh>

#include <cassert>

namespace {

class TypeChecker : public Visitor {
    Program *const m_program;
    Function *m_function{nullptr};
    BasicBlock *m_block{nullptr};
    BasicBlock::iterator m_insert_pos{nullptr};

    Value *build_coerce_cast(Value *value, const Type *type, CastOp op);
    Value *coerce(Value *value, const Type *type);

public:
    explicit TypeChecker(Program *program) : m_program(program) {}

    void check(Function *function);
    void visit(BinaryInst *) override;
    void visit(BranchInst *) override;
    void visit(CallInst *) override;
    void visit(CastInst *) override;
    void visit(CompareInst *) override;
    void visit(CondBranchInst *) override;
    void visit(LoadInst *) override;
    void visit(PhiInst *) override;
    void visit(StoreInst *) override;
    void visit(RetInst *) override;
};

const Type *resulting_type(const IntType *lhs, const IntType *rhs) {
    return lhs->bit_width() > rhs->bit_width() ? lhs : rhs;
}

const Type *resulting_type(const IntType *lhs, const Type *rhs) {
    switch (rhs->kind()) {
    case TypeKind::Invalid:
        return lhs;
    case TypeKind::Int:
        return resulting_type(lhs, rhs->as<IntType>());
    case TypeKind::Pointer:
        assert(false);
    default:
        assert(false);
    }
}

const Type *resulting_type(const Type *lhs, const Type *rhs) {
    if (lhs == rhs) {
        return lhs;
    }
    switch (lhs->kind()) {
    case TypeKind::Int:
        return resulting_type(lhs->as<IntType>(), rhs);
    case TypeKind::Pointer:
        assert(false);
    default:
        assert(false);
    }
}

Value *TypeChecker::build_coerce_cast(Value *value, const Type *type, CastOp op) {
    if (auto *constant = value->as<Constant>()) {
        auto new_constant = new Constant(constant->value());
        new_constant->set_type(type);
        return new_constant;
    }
    return m_block->insert<CastInst>(m_insert_pos, op, type, value);
}

Value *TypeChecker::coerce(Value *value, const Type *type) {
    assert(!type->is<InvalidType>());
    if (value->type() == type) {
        return value;
    }
    if (value->type()->is<InvalidType>()) {
        return build_coerce_cast(value, type, CastOp::Extend);
    }
    if (value->type()->is<IntType>() && type->is<IntType>()) {
        const auto *from_type = value->type()->as<IntType>();
        const auto *to_type = type->as<IntType>();
        assert(to_type->bit_width() > from_type->bit_width());
        return build_coerce_cast(value, to_type, CastOp::Extend);
    }
    assert(false);
}

void TypeChecker::check(Function *function) {
    m_function = function;
    assert(function->return_type() != nullptr);
    for (auto *arg : function->args()) {
        assert(arg->has_type());
    }
    for (auto *var : function->vars()) {
        var->set_type(PointerType::get(var->var_type()));
    }
    for (auto *block : *function) {
        m_block = block;
        m_insert_pos = m_block->begin();
        for (auto *inst : *block) {
            inst->accept(this);
            ++m_insert_pos;
        }
    }
}

void TypeChecker::visit(BinaryInst *binary) {
    auto *lhs = binary->lhs();
    auto *rhs = binary->rhs();
    const auto *type = resulting_type(lhs->type(), rhs->type());
    binary->set_type(type);
    lhs->replace_all_uses_with(coerce(lhs, type));
    rhs->replace_all_uses_with(coerce(rhs, type));
}

void TypeChecker::visit(BranchInst *) {
    assert(false);
}

void TypeChecker::visit(CallInst *call) {
    auto *callee = call->callee();
    assert(call->args().size() == callee->args().size());
    for (int i = 0; auto *param : callee->args()) {
        auto *arg = call->args().at(i++);
        call->replace_uses_of_with(arg, coerce(arg, param->type()));
    }
    call->set_type(callee->return_type());
}

void TypeChecker::visit(CastInst *) {
    assert(false);
}

void TypeChecker::visit(CompareInst *) {
    assert(false);
}

void TypeChecker::visit(CondBranchInst *) {
    assert(false);
}

void TypeChecker::visit(LoadInst *load) {
    assert(load->ptr()->type()->is<PointerType>());
    const auto *ptr_type = load->ptr()->type()->as<PointerType>();
    load->set_type(ptr_type->pointee_type());
}

void TypeChecker::visit(PhiInst *) {
    assert(false);
}

void TypeChecker::visit(StoreInst *store) {
    assert(store->ptr()->type()->is<PointerType>());
    const auto *ptr_type = store->ptr()->type()->as<PointerType>();
    const auto *type = resulting_type(ptr_type->pointee_type(), store->val()->type());
    store->replace_uses_of_with(store->val(), coerce(store->val(), type));
}

void TypeChecker::visit(RetInst *ret) {
    const auto *expected_type = m_function->return_type();
    ret->replace_uses_of_with(ret->val(), coerce(ret->val(), expected_type));
}

} // namespace

void type_check(Program *program) {
    TypeChecker checker(program);
    for (auto *function : *program) {
        checker.check(function);
    }
}
