#pragma once

#include <support/Assert.hh>
#include <support/Castable.hh>
#include <support/ListNode.hh>

namespace ast {

enum class NodeKind {
    AsmExpr,
    AssignExpr,
    BinExpr,
    Block,
    CallExpr,
    CastExpr,
    ConstDecl,
    ConstructExpr,
    DeclStmt,
    FunctionArg,
    FunctionDecl,
    IfStmt,
    ImportStmt,
    MemberExpr,
    NumLit,
    PointerType,
    RetStmt,
    Root,
    StringLit,
    StructField,
    StructType,
    Symbol,
    TraitType,
    TypeDecl,
    UnaryExpr,
};

class Node : public Castable<Node, NodeKind, false>, public ListNode {
    const NodeKind m_kind;
    const int m_line;

protected:
    Node(NodeKind kind, int line) : m_kind(kind), m_line(line) {}

public:
    NodeKind kind() const { return m_kind; }
    int line() const { return m_line; }
};

} // namespace ast
