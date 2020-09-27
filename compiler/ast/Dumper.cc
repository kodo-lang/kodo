#include <ast/Dumper.hh>

#include <ast/Nodes.hh>
#include <ast/Visitor.hh>

#include <iostream>

namespace ast {

namespace {

struct Dumper : public Visitor {
    void visit(const AssignStmt *) override;
    void visit(const BinExpr *) override;
    void visit(const CallExpr *) override;
    void visit(const DeclStmt *) override;
    void visit(const FunctionArg *) override;
    void visit(const FunctionDecl *) override;
    void visit(const NumLit *) override;
    void visit(const RetStmt *) override;
    void visit(const Root *) override;
    void visit(const UnaryExpr *) override;
    void visit(const VarExpr *) override;
};

void Dumper::visit(const AssignStmt *assign_stmt) {
    std::cout << "AssignStmt(";
    std::cout << assign_stmt->name() << ", ";
    assign_stmt->val()->accept(this);
    std::cout << ')';
}

void Dumper::visit(const BinExpr *bin_expr) {
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
    bin_expr->lhs()->accept(this);
    std::cout << ", ";
    bin_expr->rhs()->accept(this);
    std::cout << ')';
}

void Dumper::visit(const CallExpr *call_expr) {
    std::cout << "CallExpr(";
    std::cout << call_expr->name();
    for (const auto *arg : call_expr->args()) {
        std::cout << ", ";
        arg->accept(this);
    }
    std::cout << ')';
}

void Dumper::visit(const DeclStmt *decl_stmt) {
    std::cout << "DeclStmt(";
    std::cout << decl_stmt->name();
    if (decl_stmt->init_val() != nullptr) {
        std::cout << ", ";
        decl_stmt->init_val()->accept(this);
    }
    std::cout << ')';
}

void Dumper::visit(const FunctionArg *function_arg) {
    std::cout << "FunctionArg(";
    std::cout << function_arg->name();
    std::cout << ')';
}

void Dumper::visit(const FunctionDecl *function_decl) {
    std::cout << "FunctionDecl(";
    std::cout << function_decl->name();
    for (const auto *arg : function_decl->args()) {
        std::cout << ", ";
        arg->accept(this);
    }
    std::cout << ')';
    for (const auto *stmt : function_decl->stmts()) {
        std::cout << "\n  ";
        stmt->accept(this);
    }
}

void Dumper::visit(const NumLit *num_lit) {
    std::cout << "NumLit(";
    std::cout << std::to_string(num_lit->value());
    std::cout << ')';
}

void Dumper::visit(const RetStmt *ret_stmt) {
    std::cout << "RetStmt(";
    ret_stmt->val()->accept(this);
    std::cout << ')';
}

void Dumper::visit(const Root *root) {
    for (const auto *function : root->functions()) {
        function->accept(this);
        std::cout << '\n';
    }
}

void Dumper::visit(const UnaryExpr *unary_expr) {
    std::cout << "UnaryExpr(";
    switch (unary_expr->op()) {
    case UnaryOp::AddressOf:
        std::cout << "AddressOf";
        break;
    case UnaryOp::Deref:
        std::cout << "Deref";
        break;
    }
    std::cout << ", ";
    unary_expr->val()->accept(this);
    std::cout << ')';
}

void Dumper::visit(const VarExpr *var_expr) {
    std::cout << "VarExpr(";
    std::cout << var_expr->name();
    std::cout << ')';
}

} // namespace

void dump(const Root *root) {
    Dumper dumper;
    root->accept(&dumper);
}

} // namespace ast
