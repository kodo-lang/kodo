#include <CodeGen.hh>

#include <cassert>

CodeGen::CodeGen(llvm::Module *module) : m_module(module), m_builder(module->getContext()) {}

llvm::Type *CodeGen::llvm_type(const Type &type) {
    switch (type.kind) {
    case TypeKind::Invalid:
        assert(false);
    case TypeKind::Int:
        return llvm::Type::getIntNTy(m_module->getContext(), type.bit_width);
    }
}

void CodeGen::visit(BinExpr *bin_expr) {
    auto translate = [&](const BinExpr *bin_expr) {
        switch (bin_expr->op()) {
        case BinOp::Add:
            return m_builder.CreateAdd(m_stack.pop(), m_stack.pop());
        case BinOp::Sub:
            return m_builder.CreateSub(m_stack.pop(), m_stack.pop());
        case BinOp::Mul:
            return m_builder.CreateMul(m_stack.pop(), m_stack.pop());
        case BinOp::Div:
            return m_builder.CreateSDiv(m_stack.pop(), m_stack.pop());
        }
    };
    accept(bin_expr->rhs());
    accept(bin_expr->lhs());
    m_stack.push(translate(bin_expr));
}

void CodeGen::visit(FunctionDecl *function_decl) {
    m_function = llvm::Function::Create(llvm::FunctionType::get(llvm::Type::getInt32Ty(m_module->getContext()), false),
                                        llvm::Function::ExternalLinkage, function_decl->name(), m_module);
    m_block = llvm::BasicBlock::Create(m_module->getContext(), "entry", m_function);
    m_builder.SetInsertPoint(m_block);
    for (auto *stmt : function_decl->stmts()) {
        accept(stmt);
    }
}

void CodeGen::visit(NumLit *num_lit) {
    m_stack.push(llvm::ConstantInt::get(llvm_type(num_lit->type()), num_lit->value()));
}

void CodeGen::visit(RetStmt *ret_stmt) {
    accept(ret_stmt->val());
    m_builder.CreateRet(m_stack.pop());
}
