#pragma once

#include <support/HasKind.hh>
#include <support/ListNode.hh>
#include <support/Assert.hh>

namespace ast {

class Visitor;

enum class NodeKind {
    AssignExpr,
    BinExpr,
    Block,
    CallExpr,
    CastExpr,
    DeclStmt,
    FunctionArg,
    FunctionDecl,
    IfStmt,
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

    template <typename T>
    const T *as() const requires HasKind<T, NodeKind>;

    NodeKind kind() const { return m_kind; }
    int line() const { return m_line; }
};

template <typename T>
const T *Node::as() const requires HasKind<T, NodeKind> {
    ASSERT(m_kind == T::KIND);
    ASSERT_PEDANTIC(dynamic_cast<const T *>(this) != nullptr);
    return static_cast<const T *>(this);
}

} // namespace ast
