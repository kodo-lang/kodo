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
    const int m_line;

protected:
    Node(NodeKind kind, int line) : m_kind(kind), m_line(line) {}

public:
    virtual void accept(Visitor *visitor) const = 0;

    NodeKind kind() const { return m_kind; }
    int line() const { return m_line; }
};

} // namespace ast
