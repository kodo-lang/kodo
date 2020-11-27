#pragma once

#include <ast/Node.hh>
#include <support/Box.hh>
#include <support/List.hh>

#include <string>
#include <utility>

namespace ast {

class Symbol;

class AsmExpr : public Node {
    const std::string m_instruction;
    std::vector<std::string> m_clobbers;
    std::vector<std::pair<std::string, Box<const Node>>> m_inputs;
    std::vector<std::pair<std::string, Box<const Node>>> m_outputs;

public:
    static constexpr auto KIND = NodeKind::AsmExpr;

    AsmExpr(int line, std::string instruction) : Node(KIND, line), m_instruction(std::move(instruction)) {}

    void accept(Visitor *visitor) const override;
    void add_clobber(std::string clobber) { m_clobbers.push_back(std::move(clobber)); }
    void add_input(std::string input, const Node *expr) { m_inputs.emplace_back(std::move(input), expr); }
    void add_output(std::string output, const Node *expr) { m_outputs.emplace_back(std::move(output), expr); }

    const std::string &instruction() const { return m_instruction; }
    const std::vector<std::string> &clobbers() const { return m_clobbers; }
    const std::vector<std::pair<std::string, Box<const Node>>> &inputs() const { return m_inputs; }
    const std::vector<std::pair<std::string, Box<const Node>>> &outputs() const { return m_outputs; }
};

class AssignExpr : public Node {
    const Box<const Node> m_lhs;
    const Box<const Node> m_rhs;

public:
    static constexpr auto KIND = NodeKind::AssignExpr;

    AssignExpr(int line, const Node *lhs, const Node *rhs) : Node(KIND, line), m_lhs(lhs), m_rhs(rhs) {}

    void accept(Visitor *visitor) const override;

    const Node *lhs() const { return *m_lhs; }
    const Node *rhs() const { return *m_rhs; }
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
    const Box<const Node> m_lhs;
    const Box<const Node> m_rhs;

public:
    static constexpr auto KIND = NodeKind::BinExpr;

    BinExpr(int line, BinOp op, const Node *lhs, const Node *rhs)
        : Node(KIND, line), m_op(op), m_lhs(lhs), m_rhs(rhs) {}

    void accept(Visitor *visitor) const override;

    BinOp op() const { return m_op; }
    const Node *lhs() const { return *m_lhs; }
    const Node *rhs() const { return *m_rhs; }
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
    Box<const Symbol> m_name;
    List<const Node> m_args;

public:
    static constexpr auto KIND = NodeKind::CallExpr;

    CallExpr(int line, const Symbol *name) : Node(KIND, line), m_name(name) {}

    void accept(Visitor *visitor) const override;
    void add_arg(const Node *arg) { m_args.insert(m_args.end(), arg); }

    const Symbol *name() const { return *m_name; }
    const List<const Node> &args() const { return m_args; }
};

class CastExpr : public Node {
    const Box<const Node> m_type;
    const Box<const Node> m_val;

public:
    static constexpr auto KIND = NodeKind::CastExpr;

    CastExpr(int line, const Node *type, const Node *val) : Node(KIND, line), m_type(type), m_val(val) {}

    void accept(Visitor *visitor) const override;

    const Node *type() const { return *m_type; }
    const Node *val() const { return *m_val; }
};

class ConstDecl : public Node {
    const std::string m_name;
    const Box<const Node> m_type;
    const Box<const Node> m_init_val;

public:
    static constexpr auto KIND = NodeKind::ConstDecl;

    ConstDecl(int line, std::string name, const Node *type, const Node *init_val)
        : Node(KIND, line), m_name(std::move(name)), m_type(type), m_init_val(init_val) {}

    void accept(Visitor *visitor) const override;

    const std::string &name() const { return m_name; }
    const Node *type() const { return *m_type; }
    const Node *init_val() const { return *m_init_val; }
};

class ConstructExpr : public Node {
    const std::string m_name;
    List<const Node> m_args;

public:
    static constexpr auto KIND = NodeKind::ConstructExpr;

    ConstructExpr(int line, std::string name) : Node(KIND, line), m_name(std::move(name)) {}

    void accept(Visitor *visitor) const override;
    void add_arg(const Node *arg) { m_args.insert(m_args.end(), arg); }

    const std::string &name() const { return m_name; }
    const List<const Node> &args() const { return m_args; }
};

class DeclStmt : public Node {
    const std::string m_name;
    const Box<const Node> m_type;
    const Box<const Node> m_init_val;
    const bool m_is_mutable;

public:
    static constexpr auto KIND = NodeKind::DeclStmt;

    DeclStmt(int line, std::string name, const Node *type, const Node *init_val, bool is_mutable)
        : Node(KIND, line), m_name(std::move(name)), m_type(type), m_init_val(init_val), m_is_mutable(is_mutable) {}

    void accept(Visitor *visitor) const override;

    const std::string &name() const { return m_name; }
    const Node *type() const { return *m_type; }
    const Node *init_val() const { return *m_init_val; }
    bool is_mutable() const { return m_is_mutable; }
};

class FunctionArg : public Node {
    const std::string m_name;
    const Box<const Node> m_type;
    const bool m_is_mutable;

public:
    static constexpr auto KIND = NodeKind::FunctionArg;

    FunctionArg(int line, std::string name, const Node *type, bool is_mutable)
        : Node(KIND, line), m_name(std::move(name)), m_type(type), m_is_mutable(is_mutable) {}

    void accept(Visitor *visitor) const override;

    const std::string &name() const { return m_name; }
    const Node *type() const { return *m_type; }
    bool is_mutable() const { return m_is_mutable; }
};

class FunctionDecl : public Node {
    const Box<const Symbol> m_name;
    const bool m_externed;
    const bool m_instance;
    List<const FunctionArg> m_args;
    Box<const Block> m_block;
    Box<const Node> m_return_type;

public:
    static constexpr auto KIND = NodeKind::FunctionDecl;

    FunctionDecl(int line, const Symbol *name, bool externed, bool instance)
        : Node(KIND, line), m_name(name), m_externed(externed), m_instance(instance) {}

    void accept(Visitor *visitor) const override;
    void set_block(const Block *block) { m_block = block; }
    void set_return_type(const Node *return_type) { m_return_type = return_type; }

    template <typename... Args>
    FunctionArg *add_arg(Args &&... args) {
        return m_args.emplace<FunctionArg>(m_args.end(), std::forward<Args>(args)...);
    }

    const Symbol *name() const { return *m_name; }
    bool externed() const { return m_externed; }
    bool instance() const { return m_instance; }
    const List<const FunctionArg> &args() const { return m_args; }
    const Block *block() const { return *m_block; }
    const Node *return_type() const { return *m_return_type; }
};

class IfStmt : public Node {
    const Box<const Node> m_expr;
    Box<const Block> m_block;

public:
    static constexpr auto KIND = NodeKind::IfStmt;

    IfStmt(int line, const Node *expr, const Block *block) : Node(KIND, line), m_expr(expr), m_block(block) {}

    void accept(Visitor *visitor) const override;

    const Node *expr() const { return *m_expr; }
    const Block *block() const { return *m_block; }
};

class ImportStmt : public Node {
    const std::string m_path;

public:
    static constexpr auto KIND = NodeKind::ImportStmt;

    ImportStmt(int line, std::string path) : Node(KIND, line), m_path(std::move(path)) {}

    void accept(Visitor *visitor) const override;

    const std::string &path() const { return m_path; }
};

class MemberExpr : public Node {
    const Box<const Node> m_lhs;
    const Box<const Node> m_rhs;
    const bool m_is_pointer;

public:
    static constexpr auto KIND = NodeKind::MemberExpr;

    MemberExpr(int line, const Node *lhs, const Node *rhs, bool is_pointer)
        : Node(KIND, line), m_lhs(lhs), m_rhs(rhs), m_is_pointer(is_pointer) {}

    void accept(Visitor *visitor) const override;

    const Node *lhs() const { return *m_lhs; }
    const Node *rhs() const { return *m_rhs; }
    bool is_pointer() const { return m_is_pointer; }
};

class NumLit : public Node {
    const std::uint64_t m_value;

public:
    static constexpr auto KIND = NodeKind::NumLit;

    NumLit(int line, std::uint64_t value) : Node(KIND, line), m_value(value) {}

    void accept(Visitor *visitor) const override;

    std::uint64_t value() const { return m_value; }
};

class PointerType : public Node {
    const Box<const Node> m_pointee_type;
    const bool m_is_mutable;

public:
    static constexpr auto KIND = NodeKind::PointerType;

    PointerType(int line, const Node *pointee_type, bool is_mutable)
        : Node(KIND, line), m_pointee_type(pointee_type), m_is_mutable(is_mutable) {}

    void accept(Visitor *visitor) const override;

    const Node *pointee_type() const { return *m_pointee_type; }
    bool is_mutable() const { return m_is_mutable; }
};

class RetStmt : public Node {
    const Box<const Node> m_val;

public:
    static constexpr auto KIND = NodeKind::RetStmt;

    RetStmt(int line, const Node *val) : Node(KIND, line), m_val(val) {}

    void accept(Visitor *visitor) const override;

    const Node *val() const { return *m_val; }
};

class Root : public Node {
    List<const Node> m_decls;

public:
    static constexpr auto KIND = NodeKind::Root;

    Root() : Node(KIND, 0) {}

    void accept(Visitor *visitor) const override;

    template <typename T, typename... Args>
    T *add(Args &&... args) {
        return m_decls.emplace<T>(m_decls.end(), std::forward<Args>(args)...);
    }

    const List<const Node> &decls() const { return m_decls; }
};

class StringLit : public Node {
    const std::string m_value;

public:
    static constexpr auto KIND = NodeKind::StringLit;

    StringLit(int line, std::string value) : Node(KIND, line), m_value(std::move(value)) {}

    void accept(Visitor *visitor) const override;

    const std::string &value() const { return m_value; }
};

class StructField : public Node {
    const std::string m_name;
    const Box<const Node> m_type;

public:
    static constexpr auto KIND = NodeKind::StructField;

    StructField(int line, std::string name, const Node *type)
        : Node(KIND, line), m_name(std::move(name)), m_type(type) {}

    void accept(Visitor *visitor) const override;

    const std::string &name() const { return m_name; }
    const Node *type() const { return *m_type; }
};

class StructType : public Node {
    List<const StructField> m_fields;

public:
    static constexpr auto KIND = NodeKind::StructType;

    explicit StructType(int line) : Node(KIND, line) {}

    void accept(Visitor *visitor) const override;

    template <typename... Args>
    void add_field(Args &&... args) {
        m_fields.emplace<StructField>(m_fields.end(), std::forward<Args>(args)...);
    }

    const List<const StructField> &fields() const { return m_fields; }
};

class Symbol : public Node {
    const std::vector<std::string> m_parts;

public:
    static constexpr auto KIND = NodeKind::Symbol;

    Symbol(int line, std::vector<std::string> &&parts) : Node(KIND, line), m_parts(std::move(parts)) {}

    void accept(Visitor *visitor) const override;

    const std::vector<std::string> &parts() const { return m_parts; }
};

class TypeDecl : public Node {
    const std::string m_name;
    const Box<const Node> m_type;

public:
    static constexpr auto KIND = NodeKind::TypeDecl;

    TypeDecl(int line, std::string name, const Node *type) : Node(KIND, line), m_name(std::move(name)), m_type(type) {}

    void accept(Visitor *visitor) const override;

    const std::string &name() const { return m_name; }
    const Node *type() const { return *m_type; }
};

enum class UnaryOp {
    AddressOf,
    Deref,
};

class UnaryExpr : public Node {
    const UnaryOp m_op;
    const Box<const Node> m_val;

public:
    static constexpr auto KIND = NodeKind::UnaryExpr;

    UnaryExpr(int line, UnaryOp op, const Node *val) : Node(KIND, line), m_op(op), m_val(val) {}

    void accept(Visitor *visitor) const override;

    UnaryOp op() const { return m_op; }
    const Node *val() const { return *m_val; }
};

} // namespace ast
