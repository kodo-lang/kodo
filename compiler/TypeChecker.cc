#include <TypeChecker.hh>

#include <ir/BasicBlock.hh>
#include <ir/Function.hh>
#include <ir/Instructions.hh>
#include <ir/Program.hh>
#include <ir/Visitor.hh>

#include <cassert>

namespace {

class TypeChecker : public Visitor {
    Program *const m_program;
    Function *m_function{nullptr};

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
        for (auto *inst : *block) {
            inst->accept(this);
        }
    }
}

void TypeChecker::visit(BinaryInst *binary) {
    assert(binary->lhs()->has_type());
    assert(binary->rhs()->has_type());
    assert(binary->lhs()->type() == binary->rhs()->type());
    binary->set_type(binary->lhs()->type());
}

void TypeChecker::visit(BranchInst *) {
    assert(false);
}

void TypeChecker::visit(CallInst *call) {
    auto *callee = call->callee();
    assert(call->args().size() == callee->args().size());
    for (int i = 0; auto *param : callee->args()) {
        auto *arg = call->args().at(i++);
        assert(arg->type() == param->type());
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
    const auto *ptr_type = load->ptr()->type();
    assert(ptr_type->is<PointerType>());
    load->set_type(ptr_type->as<PointerType>()->pointee_type());
}

void TypeChecker::visit(PhiInst *) {
    assert(false);
}

void TypeChecker::visit(StoreInst *store) {
    assert(store->ptr()->type() == PointerType::get(store->val()->type()));
}

void TypeChecker::visit(RetInst *ret) {
    const auto *expected_type = m_function->return_type();
    assert(ret->val()->type() == expected_type);
}

} // namespace

void type_check(Program *program) {
    TypeChecker checker(program);
    for (auto *function : *program) {
        checker.check(function);
    }
}
