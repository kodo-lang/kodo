#include <ast/Dumper.hh>

#include <ast/Nodes.hh>
#include <ast/Visitor.hh>
#include <support/Assert.hh>

#include <iostream>

namespace ast {
namespace {

class Dumper : public Visitor {
    int m_indent_width{0};

public:
    void visit(const AsmExpr *) override;
    void visit(const AssignExpr *) override;
    void visit(const BinExpr *) override;
    void visit(const Block *) override;
    void visit(const CallExpr *) override;
    void visit(const CastExpr *) override;
    void visit(const ConstructExpr *) override;
    void visit(const DeclStmt *) override;
    void visit(const FunctionArg *) override;
    void visit(const FunctionDecl *) override;
    void visit(const IfStmt *) override;
    void visit(const ImportStmt *) override;
    void visit(const MemberExpr *) override;
    void visit(const NumLit *) override;
    void visit(const PointerType *) override;
    void visit(const RetStmt *) override;
    void visit(const Root *) override;
    void visit(const StringLit *) override;
    void visit(const StructField *) override;
    void visit(const StructType *) override;
    void visit(const Symbol *) override;
    void visit(const TypeDecl *) override;
    void visit(const UnaryExpr *) override;
};

void Dumper::visit(const AsmExpr *) {
    // TODO
    std::cout << "AsmExpr()";
}

void Dumper::visit(const AssignExpr *assign_stmt) {
    std::cout << "AssignExpr(";
    assign_stmt->lhs()->accept(this);
    std::cout << ", ";
    assign_stmt->rhs()->accept(this);
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
    case BinOp::LessThan:
        std::cout << "LessThan";
        break;
    case BinOp::GreaterThan:
        std::cout << "GreaterThan";
        break;
    default:
        ASSERT_NOT_REACHED();
    }
    std::cout << ", ";
    bin_expr->lhs()->accept(this);
    std::cout << ", ";
    bin_expr->rhs()->accept(this);
    std::cout << ')';
}

void Dumper::visit(const Block *block) {
    m_indent_width += 2;
    for (const auto *stmt : block->stmts()) {
        std::cout << '\n';
        for (int i = 0; i < m_indent_width; i++) {
            std::cout << ' ';
        }
        stmt->accept(this);
    }
    m_indent_width -= 2;
}

void Dumper::visit(const CallExpr *call_expr) {
    std::cout << "CallExpr(";
    call_expr->name()->accept(this);
    for (const auto *arg : call_expr->args()) {
        std::cout << ", ";
        arg->accept(this);
    }
    std::cout << ')';
}

void Dumper::visit(const CastExpr *cast_expr) {
    std::cout << "CastExpr(";
    cast_expr->type()->accept(this);
    std::cout << ", ";
    cast_expr->val()->accept(this);
    std::cout << ')';
}

void Dumper::visit(const ConstructExpr *construct_expr) {
    std::cout << "ConstructExpr(";
    std::cout << construct_expr->name();
    for (const auto *arg : construct_expr->args()) {
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
    function_decl->name()->accept(this);
    for (const auto *arg : function_decl->args()) {
        std::cout << ", ";
        arg->accept(this);
    }
    std::cout << ')';
    if (!function_decl->externed()) {
        ASSERT(function_decl->block() != nullptr);
        function_decl->block()->accept(this);
    }
}

void Dumper::visit(const IfStmt *if_stmt) {
    std::cout << "IfStmt(";
    if_stmt->expr()->accept(this);
    std::cout << ')';
    if_stmt->block()->accept(this);
}

void Dumper::visit(const ImportStmt *import_stmt) {
    std::cout << "ImportStmt(";
    std::cout << import_stmt->path();
    std::cout << ')';
}

void Dumper::visit(const MemberExpr *member_expr) {
    std::cout << "MemberExpr(";
    member_expr->lhs()->accept(this);
    std::cout << ", " << (member_expr->is_pointer() ? "->" : ".");
    member_expr->rhs()->accept(this);
    std::cout << ')';
}

void Dumper::visit(const NumLit *num_lit) {
    std::cout << "NumLit(";
    std::cout << std::to_string(num_lit->value());
    std::cout << ')';
}

void Dumper::visit(const PointerType *) {}

void Dumper::visit(const RetStmt *ret_stmt) {
    std::cout << "RetStmt(";
    ret_stmt->val()->accept(this);
    std::cout << ')';
}

void Dumper::visit(const Root *root) {
    for (const auto *decl : root->decls()) {
        decl->accept(this);
        std::cout << '\n';
    }
}

void Dumper::visit(const StringLit *string_lit) {
    std::cout << "StringLit(";
    std::cout << string_lit->value();
    std::cout << ')';
}

void Dumper::visit(const StructField *) {}

void Dumper::visit(const StructType *) {}

void Dumper::visit(const Symbol *symbol) {
    std::cout << "Symbol(";
    for (bool first = true; const auto &part : symbol->parts()) {
        if (!first) {
            std::cout << ", ";
        }
        first = false;
        std::cout << part;
    }
    std::cout << ')';
}

void Dumper::visit(const TypeDecl *type_decl) {
    std::cout << "TypeDecl(";
    std::cout << type_decl->name();
    std::cout << ", ";
    type_decl->type()->accept(this);
    std::cout << ')';
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

} // namespace

void dump(const Root *root) {
    Dumper dumper;
    root->accept(&dumper);
}

} // namespace ast
