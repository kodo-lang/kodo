#pragma once

#include <Type.hh>
#include <support/ListNode.hh>

namespace ast {

class Visitor;

enum class NodeKind {
    AssignExpr,
    BinExpr,
    CallExpr,
    DeclStmt,
    FunctionArg,
    FunctionDecl,
    NumLit,
    RetStmt,
    Root,
    Symbol,
    UnaryExpr,
};

class Node : public ListNode {
    const NodeKind m_kind;
    const Type *m_type{nullptr};

protected:
    explicit Node(NodeKind kind) : m_kind(kind) {}

public:
    virtual void accept(Visitor *visitor) const = 0;
    void set_type(const Type *type) { m_type = type; }

    NodeKind kind() const { return m_kind; }
    const Type *type() const { return m_type; }
};

} // namespace ast
