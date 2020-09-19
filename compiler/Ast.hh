#pragma once

#include <cstdint>

enum class NodeKind {
    BinExpr,
    NumLit,
};

class AstNode {
    const NodeKind m_kind;

public:
    explicit AstNode(NodeKind kind) : m_kind(kind) {}

    NodeKind kind() const { return m_kind; }
};

enum class BinOp {
    Add,
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
    void accept(const AstNode *node);
    virtual void visit(const BinExpr *bin_expr) = 0;
    virtual void visit(const NumLit *num_lit) = 0;
};

struct AstPrinter : public AstVisitor {
    void visit(const BinExpr *) override;
    void visit(const NumLit *) override;
};
