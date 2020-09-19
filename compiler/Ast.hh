#pragma once

#include <Type.hh>

#include <cstdint>
#include <string>
#include <vector>

enum class NodeKind {
    BinExpr,
    FunctionDecl,
    NumLit,
    RetStmt,
};

class AstNode {
    friend class Analyser;

private:
    const NodeKind m_kind;
    Type m_type{TypeKind::Invalid};

public:
    explicit AstNode(NodeKind kind) : m_kind(kind) {}

    NodeKind kind() const { return m_kind; }
    const Type &type() const { return m_type; }
};

enum class BinOp {
    Add,
    Sub,
    Mul,
    Div,
};

class BinExpr : public AstNode {
    BinOp m_op;
    AstNode *m_lhs;
    AstNode *m_rhs;

public:
    BinExpr(BinOp op, AstNode *lhs, AstNode *rhs) : AstNode(NodeKind::BinExpr), m_op(op), m_lhs(lhs), m_rhs(rhs) {}

    BinOp op() const { return m_op; }
    AstNode *lhs() const { return m_lhs; }
    AstNode *rhs() const { return m_rhs; }
};

class FunctionDecl : public AstNode {
    std::string m_name;
    std::vector<AstNode *> m_stmts;

public:
    explicit FunctionDecl(std::string name) : AstNode(NodeKind::FunctionDecl), m_name(std::move(name)) {}

    void add_stmt(AstNode *stmt) { m_stmts.push_back(stmt); }

    const std::string &name() const { return m_name; }
    const std::vector<AstNode *> stmts() const { return m_stmts; }
};

class NumLit : public AstNode {
    std::uint64_t m_value;

public:
    explicit NumLit(std::uint64_t value) : AstNode(NodeKind::NumLit), m_value(value) {}

    std::uint64_t value() const { return m_value; }
};

class RetStmt : public AstNode {
    AstNode *m_val;

public:
    explicit RetStmt(AstNode *val) : AstNode(NodeKind::RetStmt), m_val(val) {}

    AstNode *val() const { return m_val; }
};

struct AstVisitor {
    void accept(AstNode *node);
    virtual void visit(BinExpr *bin_expr) = 0;
    virtual void visit(FunctionDecl *function_decl) = 0;
    virtual void visit(NumLit *num_lit) = 0;
    virtual void visit(RetStmt *ret_stmt) = 0;
};

struct AstPrinter : public AstVisitor {
    void visit(BinExpr *) override;
    void visit(FunctionDecl *function_decl) override;
    void visit(NumLit *) override;
    void visit(RetStmt *) override;
};
