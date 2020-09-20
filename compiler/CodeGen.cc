#include <CodeGen.hh>

#include <cassert>

CodeGen::CodeGen(llvm::Module *module) : m_module(module), m_builder(module->getContext()) {}

llvm::Value *Scope::find_var(std::string_view name) {
    for (const Scope *scope = this; scope != nullptr; scope = scope->m_parent) {
        if (scope->m_vars.contains(name)) {
            return scope->m_vars.at(name);
        }
    }
    return nullptr;
}

void Scope::put_var(std::string_view name, llvm::Value *value) {
    m_vars.emplace(name, value);
}

llvm::Type *CodeGen::llvm_type(const Type &type) {
    switch (type.kind) {
    case TypeKind::Invalid:
        assert(false);
    case TypeKind::Int:
        return llvm::Type::getIntNTy(m_module->getContext(), type.bit_width);
    }
}

void CodeGen::visit(AssignStmt *assign_stmt) {
    accept(assign_stmt->val());
    auto *var = m_scope_stack.peek().find_var(assign_stmt->name());
    assert(var != nullptr);
    m_builder.CreateStore(m_expr_stack.pop(), var);
}

void CodeGen::visit(BinExpr *bin_expr) {
    auto translate = [&](const BinExpr *bin_expr) {
        switch (bin_expr->op()) {
        case BinOp::Add:
            return m_builder.CreateAdd(m_expr_stack.pop(), m_expr_stack.pop());
        case BinOp::Sub:
            return m_builder.CreateSub(m_expr_stack.pop(), m_expr_stack.pop());
        case BinOp::Mul:
            return m_builder.CreateMul(m_expr_stack.pop(), m_expr_stack.pop());
        case BinOp::Div:
            return m_builder.CreateSDiv(m_expr_stack.pop(), m_expr_stack.pop());
        }
    };
    accept(bin_expr->rhs());
    accept(bin_expr->lhs());
    m_expr_stack.push(translate(bin_expr));
}

void CodeGen::visit(DeclStmt *decl_stmt) {
    if (decl_stmt->init_val() != nullptr) {
        accept(decl_stmt->init_val());
    }
    auto *val =
        decl_stmt->init_val() != nullptr ? m_expr_stack.pop() : llvm::UndefValue::get(llvm_type(decl_stmt->type()));
    auto *var = m_builder.CreateAlloca(llvm_type(decl_stmt->type()));
    m_builder.CreateStore(val, var);
    m_scope_stack.peek().put_var(decl_stmt->name(), var);
}

void CodeGen::visit(FunctionArg *function_arg) {
    auto *arg_val = m_function->getArg(m_arg_map.at(function_arg));
    auto *arg_var = m_builder.CreateAlloca(llvm_type(function_arg->type()));
    m_builder.CreateStore(arg_val, arg_var);
    m_scope_stack.peek().put_var(function_arg->name(), arg_var);
}

void CodeGen::visit(FunctionDecl *function_decl) {
    m_arg_map.clear();
    std::vector<llvm::Type *> arg_types;
    for (int i = 0; auto *arg : function_decl->args()) {
        arg_types.push_back(llvm_type(arg->type()));
        m_arg_map.emplace(arg, i++);
    }
    m_function = llvm::Function::Create(llvm::FunctionType::get(llvm::Type::getInt32Ty(m_module->getContext()), arg_types, false),
                                        llvm::Function::ExternalLinkage, function_decl->name(), m_module);
    m_block = llvm::BasicBlock::Create(m_module->getContext(), "entry", m_function);
    m_builder.SetInsertPoint(m_block);
    m_scope_stack.clear();
    m_scope_stack.emplace(nullptr);
    for (auto *arg : function_decl->args()) {
        accept(arg);
    }
    for (auto *stmt : function_decl->stmts()) {
        accept(stmt);
    }
}

void CodeGen::visit(NumLit *num_lit) {
    m_expr_stack.push(llvm::ConstantInt::get(llvm_type(num_lit->type()), num_lit->value()));
}

void CodeGen::visit(RetStmt *ret_stmt) {
    accept(ret_stmt->val());
    m_builder.CreateRet(m_expr_stack.pop());
}

void CodeGen::visit(VarExpr *var_expr) {
    auto *var = m_scope_stack.peek().find_var(var_expr->name());
    assert(var != nullptr);
    m_expr_stack.push(m_builder.CreateLoad(var));
}
