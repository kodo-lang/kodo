#include <Ast.hh>

#include <iostream>

void AstVisitor::accept(AstNode *node) {
    switch (node->kind()) {
    case NodeKind::AssignStmt:
        visit(static_cast<AssignStmt *>(node));
        break;
    case NodeKind::BinExpr:
        visit(static_cast<BinExpr *>(node));
        break;
    case NodeKind::DeclStmt:
        visit(static_cast<DeclStmt *>(node));
        break;
    case NodeKind::FunctionArg:
        visit(static_cast<FunctionArg *>(node));
        break;
    case NodeKind::FunctionDecl:
        visit(static_cast<FunctionDecl *>(node));
        break;
    case NodeKind::NumLit:
        visit(static_cast<NumLit *>(node));
        break;
    case NodeKind::RetStmt:
        visit(static_cast<RetStmt *>(node));
        break;
    case NodeKind::VarExpr:
        visit(static_cast<VarExpr *>(node));
        break;
    }
}

void AstPrinter::visit(AssignStmt *assign_stmt) {
    std::cout << "AssignStmt(";
    std::cout << assign_stmt->name();
    std::cout << ", ";
    accept(assign_stmt->val());
    std::cout << ")";
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

void AstPrinter::visit(DeclStmt *decl_stmt) {
    std::cout << "DeclStmt(";
    std::cout << decl_stmt->name();
    if (decl_stmt->init_val() != nullptr) {
        std::cout << ", ";
        accept(decl_stmt->init_val());
    }
    std::cout << ")";
}

void AstPrinter::visit(FunctionArg *function_arg) {
    std::cout << "FunctionArg(";
    std::cout << function_arg->name();
    std::cout << ")";
}

void AstPrinter::visit(FunctionDecl *function_decl) {
    std::cout << "FunctionDecl(";
    std::cout << function_decl->name();
    for (auto *arg : function_decl->args()) {
        std::cout << ", ";
        accept(arg);
    }
    std::cout << ")";
    for (auto *stmt : function_decl->stmts()) {
        std::cout << "\n  ";
        accept(stmt);
    }
}

void AstPrinter::visit(NumLit *num_lit) {
    std::cout << std::to_string(num_lit->value());
}

void AstPrinter::visit(RetStmt *ret_stmt) {
    std::cout << "RetStmt(";
    accept(ret_stmt->val());
    std::cout << ")";
}

void AstPrinter::visit(VarExpr *var_expr) {
    std::cout << "VarExpr(";
    std::cout << var_expr->name();
    std::cout << ")";
}
