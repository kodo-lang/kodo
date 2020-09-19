#include <Ast.hh>

#include <iostream>

void AstVisitor::accept(AstNode *node) {
    switch (node->kind()) {
    case NodeKind::BinExpr:
        visit(static_cast<BinExpr *>(node));
        break;
    case NodeKind::NumLit:
        visit(static_cast<NumLit *>(node));
        break;
    }
}

void AstPrinter::visit(BinExpr *bin_expr) {
    std::cout << "BinExpr(";
    switch (bin_expr->op()) {
    case BinOp::Add:
        std::cout << "Add";
        break;
    case BinOp::Sub:
        std::cout << "Sub";
        break;
    case BinOp::Mul:
        std::cout << "Mul";
        break;
    case BinOp::Div:
        std::cout << "Div";
        break;
    }
    std::cout << ", ";
    accept(bin_expr->lhs());
    std::cout << ", ";
    accept(bin_expr->rhs());
    std::cout << ")";
}

void AstPrinter::visit(NumLit *num_lit) {
    std::cout << std::to_string(num_lit->value());
}
