#include <Ast.hh>

#include <iostream>

void AstVisitor::accept(const AstNode *node) {
    switch (node->kind()) {
    case NodeKind::BinExpr:
        visit(static_cast<const BinExpr *>(node));
        break;
    case NodeKind::NumLit:
        visit(static_cast<const NumLit *>(node));
        break;
    }
}

void AstPrinter::visit(const BinExpr *bin_expr) {
    std::cout << "BinExpr(";
    switch (bin_expr->op()) {
    case BinOp::Add:
        std::cout << "Add";
    }
    std::cout << ", ";
    accept(bin_expr->lhs());
    std::cout << ", ";
    accept(bin_expr->rhs());
    std::cout << ")\n";
}

void AstPrinter::visit(const NumLit *num_lit) {
    std::cout << std::to_string(num_lit->value());
}
