#include <Analyser.hh>

#include <cassert>

void Analyser::visit(AssignStmt *assign_stmt) {
    accept(assign_stmt->val());
    assert(*assign_stmt->val()->type() == *m_vars.at(assign_stmt->name()));
    assign_stmt->set_type(m_vars.at(assign_stmt->name()));
}

void Analyser::visit(BinExpr *bin_expr) {
    accept(bin_expr->lhs());
    accept(bin_expr->rhs());
    assert(*bin_expr->lhs()->type() == *bin_expr->rhs()->type());
    bin_expr->set_type(bin_expr->lhs()->type());
}

void Analyser::visit(DeclStmt *decl_stmt) {
    assert(decl_stmt->type() != nullptr);
    if (decl_stmt->init_val() != nullptr) {
        accept(decl_stmt->init_val());
        assert(*decl_stmt->type() == *decl_stmt->init_val()->type());
    }
    m_vars.emplace(decl_stmt->name(), decl_stmt->type());
}

void Analyser::visit(FunctionArg *function_arg) {
    assert(function_arg->type() != nullptr);
    m_vars.emplace(function_arg->name(), function_arg->type());
}

void Analyser::visit(FunctionDecl *function_decl) {
    for (auto *arg : function_decl->args()) {
        accept(arg);
    }
    for (auto *stmt : function_decl->stmts()) {
        accept(stmt);
    }
}

void Analyser::visit(NumLit *num_lit) {
    num_lit->set_type(new IntType(32));
}

void Analyser::visit(RetStmt *ret_stmt) {
    accept(ret_stmt->val());
    ret_stmt->set_type(ret_stmt->val()->type());
}

void Analyser::visit(UnaryExpr *unary_expr) {
    accept(unary_expr->val());
    switch (unary_expr->op()) {
    case UnaryOp::AddressOf:
        unary_expr->set_type(new PointerType(unary_expr->val()->type()));
        break;
    case UnaryOp::Deref: {
        auto *pointee_type = unary_expr->val()->type()->as<PointerType>();
        assert(pointee_type != nullptr);
        unary_expr->set_type(pointee_type->pointee_type());
        break;
    }
    }
}

void Analyser::visit(VarExpr *var_expr) {
    var_expr->set_type(m_vars.at(var_expr->name()));
}
