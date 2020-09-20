#pragma once

#include <Type.hh>

#include <cstdint>
#include <string>
#include <vector>

enum class NodeKind {
    AssignStmt,
    BinExpr,
    DeclStmt,
    FunctionArg,
    FunctionDecl,
    NumLit,
    RetStmt,
    VarExpr,
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

class AssignStmt : public AstNode {
    std::string m_name;
    AstNode *m_val;

public:
    AssignStmt(std::string name, AstNode *val) : AstNode(NodeKind::AssignStmt), m_name(name), m_val(val) {}

    const std::string &name() const { return m_name; }
    AstNode *val() const { return m_val; }
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

class DeclStmt : public AstNode {
    std::string m_name;
    AstNode *m_init_val;

public:
    DeclStmt(std::string name, AstNode *init_val)
        : AstNode(NodeKind::DeclStmt), m_name(std::move(name)), m_init_val(init_val) {}

    const std::string &name() const { return m_name; }
    AstNode *init_val() const { return m_init_val; }
};

class FunctionArg : public AstNode {
    std::string m_name;

public:
    explicit FunctionArg(std::string name) : AstNode(NodeKind::FunctionArg), m_name(std::move(name)) {}

    const std::string &name() { return m_name; }
};

class FunctionDecl : public AstNode {
    std::string m_name;
    std::vector<FunctionArg *> m_args;
    std::vector<AstNode *> m_stmts;

public:
    explicit FunctionDecl(std::string name) : AstNode(NodeKind::FunctionDecl), m_name(std::move(name)) {}

    void add_arg(FunctionArg *arg) { m_args.push_back(arg); }
    void add_stmt(AstNode *stmt) { m_stmts.push_back(stmt); }

    const std::string &name() const { return m_name; }
    const std::vector<FunctionArg *> &args() const { return m_args; }
    const std::vector<AstNode *> &stmts() const { return m_stmts; }
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
    virtual void visit(DeclStmt *decl_stmt) = 0;
    virtual void visit(FunctionArg *function_arg) = 0;
    virtual void visit(FunctionDecl *function_decl) = 0;
    virtual void visit(NumLit *num_lit) = 0;
    virtual void visit(RetStmt *ret_stmt) = 0;
    virtual void visit(VarExpr *var_expr) = 0;
};

struct AstPrinter : public AstVisitor {
    void visit(AssignStmt *) override;
    void visit(BinExpr *) override;
    void visit(DeclStmt *decl_stmt) override;
    void visit(FunctionArg *) override;
    void visit(FunctionDecl *) override;
    void visit(NumLit *) override;
    void visit(RetStmt *) override;
    void visit(VarExpr *) override;
};
