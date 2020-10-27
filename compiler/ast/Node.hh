#pragma once

#include <support/Assert.hh>
#include <support/Castable.hh>
#include <support/ListNode.hh>

namespace ast {

class Visitor;

enum class NodeKind {
    AsmExpr,
    AssignExpr,
    BinExpr,
    Block,
    CallExpr,
    CastExpr,
    ConstructExpr,
    DeclStmt,
    FunctionArg,
    FunctionDecl,
    IfStmt,
    MemberExpr,
    NumLit,
    RetStmt,
    Root,
    StringLit,
    Symbol,
    TypeDecl,
    UnaryExpr,
};

class Node : public Castable<Node, NodeKind, false>, public ListNode {
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
