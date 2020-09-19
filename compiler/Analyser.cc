#include <Analyser.hh>

#include <cassert>

void Analyser::visit(BinExpr *bin_expr) {
    accept(bin_expr->lhs());
    accept(bin_expr->rhs());
    assert(bin_expr->lhs()->m_type == bin_expr->rhs()->m_type);
    bin_expr->m_type = bin_expr->lhs()->m_type;
}

void Analyser::visit(NumLit *num_lit) {
    num_lit->m_type.kind = TypeKind::Int;
    num_lit->m_type.bit_width = 32;
}
