#include <TypeChecker.hh>

#include <ir/BasicBlock.hh>
#include <ir/Constant.hh>
#include <ir/Function.hh>
#include <ir/Instructions.hh>
#include <ir/Program.hh>
#include <ir/Visitor.hh>
#include <support/Assert.hh>

#include <fmt/color.h>
#include <fmt/core.h>

#include <string>
#include <vector>

namespace {

class TypeChecker : public ir::Visitor {
    ir::Program *const m_program;
    ir::Function *m_function{nullptr};
    ir::BasicBlock *m_block{nullptr};
    ir::Instruction *m_instruction{nullptr};
    ir::BasicBlock::iterator m_insert_pos{nullptr};
    std::vector<std::string> m_errors;

    template <typename FmtStr, typename... Args>
    void add_error(const ir::Instruction *inst, const FmtStr &fmt, const Args &... args);

    ir::Value *build_coerce_cast(ir::Value *value, const Type *type, ir::CastOp op);
    ir::Value *coerce(ir::Value *value, const Type *type);

public:
    explicit TypeChecker(ir::Program *program) : m_program(program) {}

    void check(ir::Function *);
    void visit(ir::BinaryInst *) override;
    void visit(ir::BranchInst *) override;
    void visit(ir::CallInst *) override;
    void visit(ir::CastInst *) override;
    void visit(ir::CompareInst *) override;
    void visit(ir::CondBranchInst *) override;
    void visit(ir::LoadInst *) override;
    void visit(ir::PhiInst *) override;
    void visit(ir::StoreInst *) override;
    void visit(ir::RetInst *) override;

    const std::vector<std::string> &errors() { return m_errors; }
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
        ENSURE_NOT_REACHED();
    default:
        ENSURE_NOT_REACHED();
    }
}

const Type *resulting_type(const Type *lhs, const Type *rhs) {
    if (lhs == rhs) {
        // TODO: Hacky. resulting_type should take in two values and have special handling for constants.
        if (lhs->is<InvalidType>()) {
            return IntType::get(32);
        }
        return lhs;
    }
    switch (lhs->kind()) {
    case TypeKind::Int:
        return resulting_type(lhs->as<IntType>(), rhs);
    case TypeKind::Pointer:
        ENSURE_NOT_REACHED();
    default:
        ENSURE_NOT_REACHED();
    }
}

template <typename FmtStr, typename... Args>
void TypeChecker::add_error(const ir::Instruction *inst, const FmtStr &fmt, const Args &... args) {
    auto formatted = fmt::format(fmt, args...);
    auto error = fmt::format(fmt::fg(fmt::color::orange_red), "error:");
    m_errors.push_back(fmt::format("{} {} on line {}\n", error, formatted, inst->line()));
}

ir::Value *TypeChecker::build_coerce_cast(ir::Value *value, const Type *type, ir::CastOp op) {
    if (auto *constant = value->as_or_null<ir::Constant>()) {
        auto new_constant = new ir::Constant(constant->value());
        new_constant->set_type(type);
        return new_constant;
    }
    return m_block->insert<ir::CastInst>(m_insert_pos, op, type, value);
}

ir::Value *TypeChecker::coerce(ir::Value *value, const Type *type) {
    ASSERT(!type->is<InvalidType>());
    if (value->type() == type) {
        return value;
    }
    if (value->type()->is<InvalidType>()) {
        return build_coerce_cast(value, type, ir::CastOp::SignExtend);
    }
    if (const auto *from = value->type()->as<IntType>()) {
        if (const auto *to = type->as<IntType>()) {
            if (from->bit_width() < to->bit_width()) {
                return build_coerce_cast(value, type, ir::CastOp::SignExtend);
            }
        }
    }
    auto *inst = value->as_or_null<ir::Instruction>();
    if (inst == nullptr) {
        inst = m_instruction;
    }
    ENSURE(inst != nullptr);
    add_error(inst, "cannot implicitly cast from '{}' to '{}'", value->type()->to_string(), type->to_string());
    return new ir::Constant(0);
}

void TypeChecker::check(ir::Function *function) {
    m_function = function;
    ASSERT(function->return_type() != nullptr);
    for (auto *arg : function->args()) {
        ASSERT(arg->has_type());
    }
    for (auto *var : function->vars()) {
        var->set_type(PointerType::get(var->var_type()));
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

void TypeChecker::visit(ir::BinaryInst *binary) {
    auto *lhs = binary->lhs();
    auto *rhs = binary->rhs();
    const auto *type = resulting_type(lhs->type(), rhs->type());
    binary->set_type(type);
    binary->replace_uses_of_with(lhs, coerce(lhs, type));
    binary->replace_uses_of_with(rhs, coerce(rhs, type));
}

void TypeChecker::visit(ir::BranchInst *) {}

void TypeChecker::visit(ir::CallInst *call) {
    auto *callee = call->callee();
    if (call->args().size() != callee->args().size()) {
        add_error(call, "'{}' requires {} arguments, but {} were passed", callee->name(), callee->args().size(),
                  call->args().size());
        return;
    }
    for (int i = 0; auto *param : callee->args()) {
        auto *arg = call->args().at(i++);
        call->replace_uses_of_with(arg, coerce(arg, param->type()));
    }
    call->set_type(callee->return_type());
}

void TypeChecker::visit(ir::CastInst *cast) {
    auto *val = cast->val();
    ENSURE(val->kind() != ir::ValueKind::Constant);
    if (val->type()->is<BoolType>() && cast->type()->is<IntType>()) {
        cast->set_op(ir::CastOp::ZeroExtend);
        return;
    }
    add_error(cast, "cannot cast from '{}' to '{}'", val->type()->to_string(), cast->type()->to_string());
}

void TypeChecker::visit(ir::CompareInst *compare) {
    auto *lhs = compare->lhs();
    auto *rhs = compare->rhs();
    const auto *type = resulting_type(lhs->type(), rhs->type());
    lhs->replace_all_uses_with(coerce(lhs, type));
    rhs->replace_all_uses_with(coerce(rhs, type));
    compare->set_type(BoolType::get());
}

void TypeChecker::visit(ir::CondBranchInst *cond_branch) {
    auto *cond = cond_branch->cond();
    cond_branch->replace_uses_of_with(cond, coerce(cond, BoolType::get()));
}

void TypeChecker::visit(ir::LoadInst *load) {
    ENSURE(load->ptr()->type()->is<PointerType>());
    const auto *ptr_type = load->ptr()->type()->as<PointerType>();
    load->set_type(ptr_type->pointee_type());
}

void TypeChecker::visit(ir::PhiInst *) {
    ASSERT_NOT_REACHED();
}

void TypeChecker::visit(ir::StoreInst *store) {
    ENSURE(store->ptr()->type()->is<PointerType>());
    const auto *ptr_type = store->ptr()->type()->as<PointerType>();
    const auto *type = resulting_type(ptr_type->pointee_type(), store->val()->type());
    store->replace_uses_of_with(store->val(), coerce(store->val(), type));
}

void TypeChecker::visit(ir::RetInst *ret) {
    const auto *expected_type = m_function->return_type();
    ret->replace_uses_of_with(ret->val(), coerce(ret->val(), expected_type));
}

} // namespace

void type_check(ir::Program *program) {
    TypeChecker checker(program);
    for (auto *function : *program) {
        checker.check(function);
    }
    for (const auto &error : checker.errors()) {
        fmt::print(error);
    }
    if (!checker.errors().empty()) {
        fmt::print(fmt::fg(fmt::color::orange_red), " note: Aborting due to previous errors\n");
        exit(1);
    }
}
