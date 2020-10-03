#pragma once

#include <Type.hh>
#include <ast/Node.hh>
#include <support/List.hh>

#include <memory>
#include <string>
#include <utility>

namespace ast {

class AssignExpr : public Node {
    const std::unique_ptr<const Node> m_lhs;
    const std::unique_ptr<const Node> m_rhs;

public:
    static constexpr auto KIND = NodeKind::AssignExpr;

    AssignExpr(int line, const Node *lhs, const Node *rhs) : Node(KIND, line), m_lhs(lhs), m_rhs(rhs) {}

    void accept(Visitor *visitor) const override;

    const Node *lhs() const { return m_lhs.get(); }
    const Node *rhs() const { return m_rhs.get(); }
};

enum class BinOp {
    // Arithmetic.
    Add,
    Sub,
    Mul,
    Div,

    // Comparison.
    LessThan,
    GreaterThan,
};

class BinExpr : public Node {
    const BinOp m_op;
    const std::unique_ptr<const Node> m_lhs;
    const std::unique_ptr<const Node> m_rhs;

public:
    static constexpr auto KIND = NodeKind::BinExpr;

    BinExpr(int line, BinOp op, const Node *lhs, const Node *rhs)
        : Node(KIND, line), m_op(op), m_lhs(lhs), m_rhs(rhs) {}

    void accept(Visitor *visitor) const override;

    BinOp op() const { return m_op; }
    const Node *lhs() const { return m_lhs.get(); }
    const Node *rhs() const { return m_rhs.get(); }
};

class Block : public Node {
    List<const Node> m_stmts;

public:
    static constexpr auto KIND = NodeKind::Block;

    explicit Block(int line) : Node(KIND, line) {}

    void accept(Visitor *visitor) const override;
    void add_stmt(const Node *stmt) { m_stmts.insert(m_stmts.end(), stmt); }

    // TODO: Remove this.
    template <typename Stmt, typename... Args>
    Stmt *add_stmt(Args &&... args) {
        return m_stmts.emplace<Stmt>(m_stmts.end(), std::forward<Args>(args)...);
    }

    const List<const Node> &stmts() const { return m_stmts; }
};

class CallExpr : public Node {
    const std::string m_name;
    List<const Node> m_args;

public:
    static constexpr auto KIND = NodeKind::CallExpr;

    CallExpr(int line, std::string name) : Node(KIND, line), m_name(std::move(name)) {}

    void accept(Visitor *visitor) const override;
    void add_arg(const Node *arg) { m_args.insert(m_args.end(), arg); }

    const std::string &name() const { return m_name; }
    const List<const Node> &args() const { return m_args; }
};

class CastExpr : public Node {
    const Type *const m_type;
    const std::unique_ptr<const Node> m_val;

public:
    static constexpr auto KIND = NodeKind::CastExpr;

    CastExpr(int line, const Type *type, const Node *val) : Node(KIND, line), m_type(type), m_val(val) {}

    void accept(Visitor *visitor) const override;

    const Type *type() const { return m_type; }
    const Node *val() const { return m_val.get(); }
};

class DeclStmt : public Node {
    const std::string m_name;
    const Type *const m_type;
    const std::unique_ptr<const Node> m_init_val;

public:
    static constexpr auto KIND = NodeKind::DeclStmt;

    DeclStmt(int line, std::string name, const Type *type, const Node *init_val)
        : Node(KIND, line), m_name(std::move(name)), m_type(type), m_init_val(init_val) {}

    void accept(Visitor *visitor) const override;

    const std::string &name() const { return m_name; }
    const Type *type() const { return m_type; }
    const Node *init_val() const { return m_init_val.get(); }
};

class FunctionArg : public Node {
    const std::string m_name;
    const Type *const m_type;

public:
    static constexpr auto KIND = NodeKind::FunctionArg;

    FunctionArg(int line, std::string name, const Type *type)
        : Node(KIND, line), m_name(std::move(name)), m_type(type) {}

    void accept(Visitor *visitor) const override;

    const std::string &name() const { return m_name; }
    const Type *type() const { return m_type; }
};

class FunctionDecl : public Node {
    const std::string m_name;
    const Type *m_return_type;
    const bool m_externed;
    List<const FunctionArg> m_args;
    std::unique_ptr<const Block> m_block;

public:
    static constexpr auto KIND = NodeKind::FunctionDecl;

    FunctionDecl(int line, std::string name, bool externed)
        : Node(KIND, line), m_name(std::move(name)), m_externed(externed) {}

    void accept(Visitor *visitor) const override;
    void set_block(const Block *block) { m_block.reset(block); }
    void set_return_type(const Type *return_type) { m_return_type = return_type; }

    template <typename... Args>
    FunctionArg *add_arg(Args &&... args) {
        return m_args.emplace<FunctionArg>(m_args.end(), std::forward<Args>(args)...);
    }

    const std::string &name() const { return m_name; }
    const Type *return_type() const { return m_return_type; }
    bool externed() const { return m_externed; }
    const List<const FunctionArg> &args() const { return m_args; }
    const Block *block() const { return m_block.get(); }
};

class NumLit : public Node {
    const std::uint64_t m_value;

public:
    static constexpr auto KIND = NodeKind::NumLit;

    NumLit(int line, std::uint64_t value) : Node(KIND, line), m_value(value) {}

    void accept(Visitor *visitor) const override;

    std::uint64_t value() const { return m_value; }
};

class RetStmt : public Node {
    const std::unique_ptr<const Node> m_val;

public:
    static constexpr auto KIND = NodeKind::RetStmt;

    RetStmt(int line, const Node *val) : Node(KIND, line), m_val(val) {}

    void accept(Visitor *visitor) const override;

    const Node *val() const { return m_val.get(); }
};

class Root : public Node {
    List<const FunctionDecl> m_functions;

public:
    static constexpr auto KIND = NodeKind::Root;

    Root() : Node(KIND, 0) {}

    void accept(Visitor *visitor) const override;

    template <typename... Args>
    FunctionDecl *add_function(Args &&... args) {
        return m_functions.emplace<FunctionDecl>(m_functions.end(), std::forward<Args>(args)...);
    }

    const List<const FunctionDecl> &functions() const { return m_functions; }
};

class Symbol : public Node {
    const std::string m_name;

public:
    static constexpr auto KIND = NodeKind::Symbol;

    Symbol(int line, std::string name) : Node(KIND, line), m_name(std::move(name)) {}

    void accept(Visitor *visitor) const override;

    const std::string &name() const { return m_name; }
};

enum class UnaryOp {
    AddressOf,
    Deref,
};

class UnaryExpr : public Node {
    const UnaryOp m_op;
    const std::unique_ptr<const Node> m_val;

public:
    static constexpr auto KIND = NodeKind::UnaryExpr;

    UnaryExpr(int line, UnaryOp op, const Node *val) : Node(KIND, line), m_op(op), m_val(val) {}

    void accept(Visitor *visitor) const override;

    UnaryOp op() const { return m_op; }
    const Node *val() const { return m_val.get(); }
};

} // namespace ast
