#pragma once

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

protected:
    explicit Node(NodeKind kind) : m_kind(kind) {}

public:
    virtual void accept(Visitor *visitor) const = 0;

    NodeKind kind() const { return m_kind; }
};

} // namespace ast
