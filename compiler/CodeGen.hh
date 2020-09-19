#pragma once

#include <Ast.hh>
#include <Stack.hh>

#include <llvm/IR/Function.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Module.h>

#include <string_view>
#include <unordered_map>

class Scope {
    const Scope *const m_parent;
    std::unordered_map<std::string_view, llvm::Value *> m_vars;

public:
    explicit Scope(const Scope *parent) : m_parent(parent) {}

    llvm::Value *find_var(std::string_view name);
    void put_var(std::string_view name, llvm::Value *value);
};

class CodeGen : public AstVisitor {
    llvm::Module *const m_module;
    llvm::IRBuilder<> m_builder;

    llvm::BasicBlock *m_block;
    llvm::Function *m_function;
    Stack<llvm::Value *> m_expr_stack;
    Stack<Scope> m_scope_stack;

    llvm::Type *llvm_type(const Type &type);

public:
    explicit CodeGen(llvm::Module *module);

    void visit(AssignStmt *) override;
    void visit(BinExpr *) override;
    void visit(DeclStmt *) override;
    void visit(FunctionDecl *) override;
    void visit(NumLit *) override;
    void visit(RetStmt *) override;
    void visit(VarExpr *) override;
};
