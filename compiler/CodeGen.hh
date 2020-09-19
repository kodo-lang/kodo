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

public:
    explicit CodeGen(llvm::Module *module);

    void visit(const BinExpr *) override;
    void visit(const NumLit *) override;
    void finish();
};
