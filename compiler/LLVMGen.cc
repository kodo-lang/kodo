#include <LLVMGen.hh>

#include <ir/BasicBlock.hh>
#include <ir/Constant.hh>
#include <ir/Function.hh>
#include <ir/Instructions.hh>
#include <ir/Program.hh>
#include <ir/Visitor.hh>

#include <llvm/IR/IRBuilder.h>

#include <cassert>
#include <utility>

namespace {

class LLVMGen : public Visitor {
    const Program *const m_program;
    llvm::LLVMContext *const m_llvm_context;

    std::unique_ptr<llvm::Module> m_llvm_module;
    llvm::Function *m_llvm_function{nullptr};
    llvm::BasicBlock *m_llvm_block{nullptr};
    llvm::IRBuilder<> m_llvm_builder;

public:
    LLVMGen(const Program *program, llvm::LLVMContext *llvm_context);

    llvm::Type *gen_type(const Type *type);
    llvm::Value *gen_constant(const Constant *constant);
    llvm::Value *gen_value(const Value *value);

    void visit(BinaryInst *) override;
    void visit(BranchInst *) override;
    void visit(CompareInst *) override;
    void visit(CondBranchInst *) override;
    void visit(LoadInst *) override;
    void visit(PhiInst *) override;
    void visit(StoreInst *) override;
    void visit(RetInst *) override;

    void gen_block(const BasicBlock *block);
    void gen_function(const Function *function);

    std::unique_ptr<llvm::Module> module() { return std::move(m_llvm_module); }
};

LLVMGen::LLVMGen(const Program *program, llvm::LLVMContext *llvm_context)
    : m_program(program), m_llvm_context(llvm_context), m_llvm_builder(*llvm_context) {
    m_llvm_module = std::make_unique<llvm::Module>("main", *llvm_context);
}

llvm::Type *LLVMGen::gen_type(const Type *type) {
    switch (type->kind()) {
    case TypeKind::Invalid:
    case TypeKind::Pointer:
        assert(false);
    case TypeKind::Int:
        return llvm::Type::getIntNTy(*m_llvm_context, type->as<IntType>()->bit_width());
    }
}

llvm::Value *LLVMGen::gen_constant(const Constant *constant) {
    return llvm::ConstantInt::get(gen_type(constant->type()), constant->value());
}

llvm::Value *LLVMGen::gen_value(const Value *value) {
    switch (value->kind()) {
    case ValueKind::Argument:
    case ValueKind::BasicBlock:
    case ValueKind::Instruction:
    case ValueKind::LocalVar:
        assert(false);
    case ValueKind::Constant:
        return gen_constant(value->as<Constant>());
    }
}

void LLVMGen::visit(BinaryInst *) {
    assert(false);
}

void LLVMGen::visit(BranchInst *) {
    assert(false);
}

void LLVMGen::visit(CompareInst *) {
    assert(false);
}

void LLVMGen::visit(CondBranchInst *) {
    assert(false);
}

void LLVMGen::visit(LoadInst *) {
    assert(false);
}

void LLVMGen::visit(PhiInst *) {
    assert(false);
}

void LLVMGen::visit(StoreInst *) {
    assert(false);
}

void LLVMGen::visit(RetInst *ret) {
    m_llvm_builder.CreateRet(gen_value(ret->val()));
}

void LLVMGen::gen_block(const BasicBlock *block) {
    m_llvm_block = llvm::BasicBlock::Create(*m_llvm_context, "", m_llvm_function);
    m_llvm_builder.SetInsertPoint(m_llvm_block);
    for (const auto *inst : *block) {
        // TODO: Remove const_cast.
        const_cast<Instruction *>(inst)->accept(this);
    }
}

void LLVMGen::gen_function(const Function *function) {
    auto *function_type = llvm::FunctionType::get(llvm::Type::getInt32Ty(*m_llvm_context), false);
    m_llvm_function =
        llvm::Function::Create(function_type, llvm::Function::ExternalLinkage, function->name(), *m_llvm_module);
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
