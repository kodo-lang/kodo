#include <CodeGen.hh>

#include <cassert>

CodeGen::CodeGen(llvm::Module *module) : m_module(module), m_builder(module->getContext()) {
    m_function = llvm::Function::Create(llvm::FunctionType::get(llvm::Type::getInt32Ty(module->getContext()), false),
                                        llvm::Function::ExternalLinkage, "main", module);
    m_block = llvm::BasicBlock::Create(module->getContext(), "entry", m_function);
    m_builder.SetInsertPoint(m_block);
}

void CodeGen::visit(const BinExpr *bin_expr) {
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

void CodeGen::visit(const NumLit *num_lit) {
    m_stack.push(llvm::ConstantInt::get(llvm::Type::getInt32Ty(m_module->getContext()), num_lit->value()));
}

void CodeGen::finish() {
    assert(m_stack.size() == 1);
    m_builder.CreateRet(m_stack.pop());
}
