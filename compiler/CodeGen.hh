#pragma once

#include <Ast.hh>
#include <Stack.hh>

#include <llvm/IR/Function.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Module.h>

class CodeGen : public AstVisitor {
    llvm::Module *const m_module;
    llvm::IRBuilder<> m_builder;

    llvm::BasicBlock *m_block;
    llvm::Function *m_function;
    Stack<llvm::Value *> m_stack;

    llvm::Type *llvm_type(const Type &type);

public:
    explicit CodeGen(llvm::Module *module);

    void visit(BinExpr *) override;
    void visit(NumLit *) override;
    void visit(RetStmt *) override;
};
