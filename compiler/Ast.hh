#pragma once

#include <Type.hh>

#include <cstdint>

enum class NodeKind {
    BinExpr,
    NumLit,
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

class NumLit : public AstNode {
    std::uint64_t m_value;

public:
    explicit NumLit(std::uint64_t value) : AstNode(NodeKind::NumLit), m_value(value) {}

    std::uint64_t value() const { return m_value; }
};

struct AstVisitor {
    void accept(AstNode *node);
    virtual void visit(BinExpr *bin_expr) = 0;
    virtual void visit(NumLit *num_lit) = 0;
};

struct AstPrinter : public AstVisitor {
    void visit(BinExpr *) override;
    void visit(NumLit *) override;
};
