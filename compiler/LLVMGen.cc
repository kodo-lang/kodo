#include <LLVMGen.hh>

#include <ir/BasicBlock.hh>
#include <ir/Constant.hh>
#include <ir/Function.hh>
#include <ir/Instructions.hh>
#include <ir/Program.hh>
#include <support/Assert.hh>

#include <llvm/IR/IRBuilder.h>

#include <unordered_map>
#include <utility>
#include <vector>

namespace {

class LLVMGen {
    const Program *const m_program;
    llvm::LLVMContext *const m_llvm_context;

    std::unique_ptr<llvm::Module> m_llvm_module;
    llvm::Function *m_llvm_function{nullptr};
    llvm::BasicBlock *m_llvm_block{nullptr};
    llvm::IRBuilder<> m_llvm_builder;

    std::unordered_map<const Argument *, llvm::Argument *> m_arg_map;
    std::unordered_map<const BasicBlock *, llvm::BasicBlock *> m_block_map;
    std::unordered_map<const Value *, llvm::Value *> m_value_map;

public:
    LLVMGen(const Program *program, llvm::LLVMContext *llvm_context);

    llvm::Type *llvm_type(const Type *);
    llvm::Value *llvm_value(const Value *);

    llvm::Value *gen_binary(const BinaryInst *);
    llvm::Value *gen_call(const CallInst *);
    llvm::Value *gen_cast(const CastInst *);
    llvm::Value *gen_compare(const CompareInst *);
    llvm::Value *gen_load(const LoadInst *);
    void gen_branch(const BranchInst *);
    void gen_cond_branch(const CondBranchInst *);
    void gen_store(const StoreInst *);
    void gen_ret(const RetInst *);

    llvm::Value *gen_argument(const Argument *);
    llvm::Value *gen_constant(const Constant *);
    llvm::Value *gen_instruction(const Instruction *);
    llvm::Value *gen_value(const Value *);

    void gen_block(const BasicBlock *);
    void gen_function(const Function *);

    std::unique_ptr<llvm::Module> module() { return std::move(m_llvm_module); }
};

LLVMGen::LLVMGen(const Program *program, llvm::LLVMContext *llvm_context)
    : m_program(program), m_llvm_context(llvm_context), m_llvm_builder(*llvm_context) {
    m_llvm_module = std::make_unique<llvm::Module>("main", *llvm_context);
}

llvm::Type *LLVMGen::llvm_type(const Type *type) {
    switch (type->kind()) {
    case TypeKind::Bool:
        return llvm::Type::getInt1Ty(*m_llvm_context);
    case TypeKind::Int:
        return llvm::Type::getIntNTy(*m_llvm_context, type->as<IntType>()->bit_width());
    case TypeKind::Pointer:
        return llvm::PointerType::get(llvm_type(type->as<PointerType>()->pointee_type()), 0);
    case TypeKind::Void:
        return llvm::Type::getVoidTy(*m_llvm_context);
    default:
        ENSURE_NOT_REACHED();
    }
}

llvm::Value *LLVMGen::llvm_value(const Value *value) {
    if (!m_value_map.contains(value)) {
        m_value_map.emplace(value, gen_value(value));
    }
    auto *llvm_value = m_value_map.at(value);
    if (value->has_name()) {
        llvm_value->setName(value->name());
    }
    return llvm_value;
}

llvm::Value *LLVMGen::gen_binary(const BinaryInst *binary) {
    auto *lhs = llvm_value(binary->lhs());
    auto *rhs = llvm_value(binary->rhs());
    switch (binary->op()) {
    case BinaryOp::Add:
        return m_llvm_builder.CreateAdd(lhs, rhs);
    case BinaryOp::Sub:
        return m_llvm_builder.CreateSub(lhs, rhs);
    case BinaryOp::Mul:
        return m_llvm_builder.CreateMul(lhs, rhs);
    case BinaryOp::Div:
        return m_llvm_builder.CreateSDiv(lhs, rhs);
    default:
        ENSURE_NOT_REACHED();
    }
}

llvm::Value *LLVMGen::gen_call(const CallInst *call) {
    std::vector<llvm::Value *> args;
    for (auto *arg : call->args()) {
        args.push_back(llvm_value(arg));
    }
    auto *callee = m_llvm_module->getFunction(call->callee()->name());
    return m_llvm_builder.CreateCall(callee, args);
}

llvm::Value *LLVMGen::gen_cast(const CastInst *cast) {
    auto *type = llvm_type(cast->type());
    auto *value = llvm_value(cast->val());
    switch (cast->op()) {
    case CastOp::SignExtend:
        return m_llvm_builder.CreateSExt(value, type);
    case CastOp::ZeroExtend:
        return m_llvm_builder.CreateZExt(value, type);
    default:
        ENSURE_NOT_REACHED();
    }
}

llvm::Value *LLVMGen::gen_compare(const CompareInst *compare) {
    auto *lhs = llvm_value(compare->lhs());
    auto *rhs = llvm_value(compare->rhs());
    switch (compare->op()) {
    case CompareOp::LessThan:
        return m_llvm_builder.CreateICmpSLT(lhs, rhs);
    case CompareOp::GreaterThan:
        return m_llvm_builder.CreateICmpSGT(lhs, rhs);
    default:
        ENSURE_NOT_REACHED();
    }
}

llvm::Value *LLVMGen::gen_load(const LoadInst *load) {
    return m_llvm_builder.CreateLoad(llvm_value(load->ptr()));
}

void LLVMGen::gen_branch(const BranchInst *branch) {
    m_llvm_builder.CreateBr(m_block_map.at(branch->dst()));
}

void LLVMGen::gen_cond_branch(const CondBranchInst *cond_branch) {
    auto *cond = llvm_value(cond_branch->cond());
    auto *true_dst = m_block_map.at(cond_branch->true_dst());
    auto *false_dst = m_block_map.at(cond_branch->false_dst());
    m_llvm_builder.CreateCondBr(cond, true_dst, false_dst);
}

void LLVMGen::gen_store(const StoreInst *store) {
    m_llvm_builder.CreateStore(llvm_value(store->val()), llvm_value(store->ptr()));
}

void LLVMGen::gen_ret(const RetInst *ret) {
    m_llvm_builder.CreateRet(llvm_value(ret->val()));
}

llvm::Value *LLVMGen::gen_argument(const Argument *argument) {
    return m_arg_map.at(argument);
}

llvm::Value *LLVMGen::gen_constant(const Constant *constant) {
    return llvm::ConstantInt::get(llvm_type(constant->type()), constant->value());
}

llvm::Value *LLVMGen::gen_instruction(const Instruction *instruction) {
    switch (instruction->inst_kind()) {
    case InstKind::Binary:
        return gen_binary(instruction->as<BinaryInst>());
    case InstKind::Branch:
        gen_branch(instruction->as<BranchInst>());
        return nullptr;
    case InstKind::Call:
        return gen_call(instruction->as<CallInst>());
    case InstKind::Cast:
        return gen_cast(instruction->as<CastInst>());
    case InstKind::Compare:
        return gen_compare(instruction->as<CompareInst>());
    case InstKind::CondBranch:
        gen_cond_branch(instruction->as<CondBranchInst>());
        return nullptr;
    case InstKind::Load:
        return gen_load(instruction->as<LoadInst>());
    case InstKind::Store:
        gen_store(instruction->as<StoreInst>());
        return nullptr;
    case InstKind::Ret:
        gen_ret(instruction->as<RetInst>());
        return nullptr;
    default:
        ENSURE_NOT_REACHED();
    }
}

llvm::Value *LLVMGen::gen_value(const Value *value) {
    switch (value->kind()) {
    case ValueKind::Argument:
        return gen_argument(value->as<Argument>());
    case ValueKind::Constant:
        return gen_constant(value->as<Constant>());
    case ValueKind::Instruction:
        return gen_instruction(value->as<Instruction>());
    default:
        ENSURE_NOT_REACHED();
    }
}

void LLVMGen::gen_block(const BasicBlock *block) {
    auto *new_block = m_block_map.at(block);
    if (m_llvm_block->empty() || !m_llvm_block->back().isTerminator()) {
        m_llvm_builder.CreateBr(new_block);
    }
    m_llvm_builder.SetInsertPoint(m_llvm_block = new_block);
    for (const auto *inst : *block) {
        llvm_value(inst);
    }
}

void LLVMGen::gen_function(const Function *function) {
    std::vector<llvm::Type *> arg_types;
    for (const auto *arg : function->args()) {
        ASSERT(arg->has_type());
        arg_types.push_back(llvm_type(arg->type()));
    }

    auto *function_type = llvm::FunctionType::get(llvm_type(function->return_type()), arg_types, false);
    m_llvm_function =
        llvm::Function::Create(function_type, llvm::Function::ExternalLinkage, function->name(), *m_llvm_module);

    // If the function has no blocks, return early.
    if (function->begin() == function->end()) {
        ASSERT(function->vars().empty());
        return;
    }

    m_llvm_block = llvm::BasicBlock::Create(*m_llvm_context, "vars", m_llvm_function);
    m_llvm_builder.SetInsertPoint(m_llvm_block);
    for (const auto *block : *function) {
        auto *llvm_block = llvm::BasicBlock::Create(*m_llvm_context, "", m_llvm_function);
        m_block_map.emplace(block, llvm_block);
    }

    for (int i = 0; const auto *arg : function->args()) {
        m_arg_map.emplace(arg, m_llvm_function->getArg(i++));
    }
    for (const auto *var : function->vars()) {
        auto *alloca = m_llvm_builder.CreateAlloca(llvm_type(var->var_type()));
        m_value_map.emplace(var, alloca);
    }
    for (const auto *block : *function) {
        gen_block(block);
    }
}

} // namespace

std::unique_ptr<llvm::Module> gen_llvm(const Program *program, llvm::LLVMContext *llvm_context) {
    LLVMGen gen(program, llvm_context);
    for (const auto *function : *program) {
        gen.gen_function(function);
    }
    return gen.module();
}
