#pragma once

#include <Type.hh>
#include <support/List.hh>
#include <support/ListNode.hh>

#include <cstdint>
#include <memory>
#include <string>
#include <utility>
#include <vector>

enum class NodeKind {
    AssignStmt,
    BinExpr,
    CallExpr,
    DeclStmt,
    FunctionArg,
    FunctionDecl,
    NumLit,
    RetStmt,
    UnaryExpr,
    VarExpr,
};

class AstNode : public ListNode {
    const NodeKind m_kind;
    Type *m_type{nullptr};

public:
    explicit AstNode(NodeKind kind) : m_kind(kind) {}

    void set_type(Type *type) { m_type = type; }

    NodeKind kind() const { return m_kind; }
    Type *type() const { return m_type; }
};

class AssignStmt : public AstNode {
    std::string m_name;
    std::unique_ptr<AstNode> m_val;

public:
    AssignStmt(std::string name, AstNode *val) : AstNode(NodeKind::AssignStmt), m_name(std::move(name)), m_val(val) {}

    const std::string &name() const { return m_name; }
    AstNode *val() const { return m_val.get(); }
};

enum class BinOp {
    Add,
    Sub,
    Mul,
    Div,
};

class BinExpr : public AstNode {
    BinOp m_op;
    std::unique_ptr<AstNode> m_lhs;
    std::unique_ptr<AstNode> m_rhs;

public:
    BinExpr(BinOp op, AstNode *lhs, AstNode *rhs) : AstNode(NodeKind::BinExpr), m_op(op), m_lhs(lhs), m_rhs(rhs) {}

    BinOp op() const { return m_op; }
    AstNode *lhs() const { return m_lhs.get(); }
    AstNode *rhs() const { return m_rhs.get(); }
};

class CallExpr : public AstNode {
    std::string m_name;
    std::vector<std::unique_ptr<AstNode>> m_args;

public:
    explicit CallExpr(std::string name) : AstNode(NodeKind::CallExpr), m_name(std::move(name)) {}

    void add_arg(AstNode *arg) { m_args.emplace_back(arg); }

    const std::string &name() const { return m_name; }
    const std::vector<std::unique_ptr<AstNode>> &args() const { return m_args; }
};

class DeclStmt : public AstNode {
    std::string m_name;
    std::unique_ptr<AstNode> m_init_val;

public:
    DeclStmt(std::string name, AstNode *init_val)
        : AstNode(NodeKind::DeclStmt), m_name(std::move(name)), m_init_val(init_val) {}

    const std::string &name() const { return m_name; }
    AstNode *init_val() const { return m_init_val.get(); }
};

class FunctionArg : public AstNode {
    std::string m_name;

public:
    explicit FunctionArg(std::string name) : AstNode(NodeKind::FunctionArg), m_name(std::move(name)) {}

    const std::string &name() { return m_name; }
};

class FunctionDecl : public AstNode {
    std::string m_name;
    List<FunctionArg> m_args;
    List<AstNode> m_stmts;

public:
    explicit FunctionDecl(std::string name) : AstNode(NodeKind::FunctionDecl), m_name(std::move(name)) {}

    template <typename... Args>
    FunctionArg *add_arg(Args &&... args) {
        return m_args.emplace<FunctionArg>(m_args.end(), std::forward<Args>(args)...);
    }

    template <typename Stmt, typename... Args>
    Stmt *add_stmt(Args &&... args) {
        return m_stmts.emplace<Stmt>(m_stmts.end(), std::forward<Args>(args)...);
    }

    void add_stmt(AstNode *stmt) {
        m_stmts.insert(m_stmts.end(), stmt);
    }

    const std::string &name() const { return m_name; }
    const List<FunctionArg> &args() const { return m_args; }
    const List<AstNode> &stmts() const { return m_stmts; }
};

class NumLit : public AstNode {
    std::uint64_t m_value;

public:
    explicit NumLit(std::uint64_t value) : AstNode(NodeKind::NumLit), m_value(value) {}

    std::uint64_t value() const { return m_value; }
};

class RetStmt : public AstNode {
    std::unique_ptr<AstNode> m_val;

public:
    explicit RetStmt(AstNode *val) : AstNode(NodeKind::RetStmt), m_val(val) {}

    AstNode *val() const { return m_val.get(); }
};

enum class UnaryOp {
    AddressOf,
    Deref,
};

class UnaryExpr : public AstNode {
    UnaryOp m_op;
    std::unique_ptr<AstNode> m_val;

public:
    UnaryExpr(UnaryOp op, AstNode *val) : AstNode(NodeKind::UnaryExpr), m_op(op), m_val(val) {}

    UnaryOp op() const { return m_op; }
    AstNode *val() const { return m_val.get(); }
};

class VarExpr : public AstNode {
    std::string m_name;

public:
    explicit VarExpr(std::string name) : AstNode(NodeKind::VarExpr), m_name(std::move(name)) {}

    const std::string &name() const { return m_name; }
};

struct AstVisitor {
    void accept(AstNode *node);
    virtual void visit(AssignStmt *assign_stmt) = 0;
    virtual void visit(BinExpr *bin_expr) = 0;
    virtual void visit(CallExpr *call_expr) = 0;
    virtual void visit(DeclStmt *decl_stmt) = 0;
    virtual void visit(FunctionArg *function_arg) = 0;
    virtual void visit(FunctionDecl *function_decl) = 0;
    virtual void visit(NumLit *num_lit) = 0;
    virtual void visit(RetStmt *ret_stmt) = 0;
    virtual void visit(UnaryExpr *unary_expr) = 0;
    virtual void visit(VarExpr *var_expr) = 0;
};

struct AstPrinter : public AstVisitor {
    void visit(AssignStmt *) override;
    void visit(BinExpr *) override;
    void visit(CallExpr *) override;
    void visit(DeclStmt *) override;
    void visit(FunctionArg *) override;
    void visit(FunctionDecl *) override;
    void visit(NumLit *) override;
    void visit(RetStmt *) override;
    void visit(UnaryExpr *) override;
    void visit(VarExpr *) override;
};
