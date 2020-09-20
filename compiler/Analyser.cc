#include <Analyser.hh>

#include <cassert>

void Analyser::visit(AssignStmt *assign_stmt) {
    accept(assign_stmt->val());
    assign_stmt->m_type.kind = TypeKind::Int;
    assign_stmt->m_type.bit_width = 32;
}

void Analyser::visit(BinExpr *bin_expr) {
    accept(bin_expr->lhs());
    accept(bin_expr->rhs());
    assert(bin_expr->lhs()->m_type == bin_expr->rhs()->m_type);
    bin_expr->m_type = bin_expr->lhs()->m_type;
}

void Analyser::visit(DeclStmt *decl_stmt) {
    if (decl_stmt->init_val() != nullptr) {
        accept(decl_stmt->init_val());
    }
    decl_stmt->m_type.kind = TypeKind::Int;
    decl_stmt->m_type.bit_width = 32;
}

void Analyser::visit(FunctionArg *function_arg) {
    function_arg->m_type.kind = TypeKind::Int;
    function_arg->m_type.bit_width = 32;
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
    num_lit->m_type.kind = TypeKind::Int;
    num_lit->m_type.bit_width = 32;
}

void Analyser::visit(RetStmt *ret_stmt) {
    accept(ret_stmt->val());
    ret_stmt->m_type = ret_stmt->val()->m_type;
}

void Analyser::visit(VarExpr *var_expr) {
    var_expr->m_type.kind = TypeKind::Int;
    var_expr->m_type.bit_width = 32;
}
