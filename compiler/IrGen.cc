#include <IrGen.hh>

#include <ast/Nodes.hh>
#include <ir/BasicBlock.hh>
#include <ir/Constant.hh>
#include <ir/Function.hh>
#include <ir/Instructions.hh>
#include <ir/Program.hh>
#include <support/Stack.hh>

#include <algorithm>
#include <cassert>
#include <string_view>
#include <unordered_map>
#include <utility>

namespace {

class Scope {
    const Scope *const m_parent;
    std::unordered_map<std::string_view, Value *> m_vars;

public:
    explicit Scope(const Scope *parent) : m_parent(parent) {}

    Value *find_var(std::string_view name);
    void put_var(std::string_view name, Value *value);
};

class IrGen {
    std::unique_ptr<Program> m_program;
    Function *m_function{nullptr};
    BasicBlock *m_block{nullptr};
    Stack<Scope> m_scope_stack;

public:
    IrGen();

    Value *gen_address_of(const ast::Node *);
    Value *gen_deref(const ast::Node *);

    Value *gen_bin_expr(const ast::BinExpr *);
    Value *gen_call_expr(const ast::CallExpr *);
    Value *gen_num_lit(const ast::NumLit *);
    Value *gen_unary_expr(const ast::UnaryExpr *);
    Value *gen_var_expr(const ast::VarExpr *);
    Value *gen_expr(const ast::Node *);

    void gen_decl_stmt(const ast::DeclStmt *);
    void gen_ret_stmt(const ast::RetStmt *);
    void gen_stmt(const ast::Node *);

    void gen_function_decl(const ast::FunctionDecl *);

    std::unique_ptr<Program> program() { return std::move(m_program); }
};

Value *Scope::find_var(std::string_view name) {
    for (const auto *scope = this; scope != nullptr; scope = scope->m_parent) {
        if (scope->m_vars.contains(name)) {
            return scope->m_vars.at(name);
        }
    }
    return nullptr;
}

void Scope::put_var(std::string_view name, Value *value) {
    m_vars.emplace(name, value);
}

IrGen::IrGen() {
    m_program = std::make_unique<Program>();
}

Value *IrGen::gen_address_of(const ast::Node *expr) {
    assert(expr->kind() == ast::NodeKind::VarExpr);
    const auto *var_expr = static_cast<const ast::VarExpr *>(expr);
    auto *var = m_scope_stack.peek().find_var(var_expr->name());
    assert(var != nullptr);
    return var;
}

Value *IrGen::gen_deref(const ast::Node *expr) {
    return m_block->append<LoadInst>(gen_expr(expr));
}

Value *IrGen::gen_bin_expr(const ast::BinExpr *bin_expr) {
    auto *lhs = gen_expr(bin_expr->lhs());
    auto *rhs = gen_expr(bin_expr->rhs());
    switch (bin_expr->op()) {
    case ast::BinOp::Add:
        return m_block->append<BinaryInst>(BinaryOp::Add, lhs, rhs);
    case ast::BinOp::Sub:
        return m_block->append<BinaryInst>(BinaryOp::Sub, lhs, rhs);
    case ast::BinOp::Mul:
        return m_block->append<BinaryInst>(BinaryOp::Mul, lhs, rhs);
    case ast::BinOp::Div:
        return m_block->append<BinaryInst>(BinaryOp::Div, lhs, rhs);
    }
}

Value *IrGen::gen_call_expr(const ast::CallExpr *call_expr) {
    auto it = std::find_if(m_program->begin(), m_program->end(), [call_expr](const Function *function) {
        return function->name() == call_expr->name();
    });
    assert(it != m_program->end());
    auto *call = m_block->append<CallInst>(*it);
    for (const auto *ast_arg : call_expr->args()) {
        call->add_arg(gen_expr(ast_arg));
    }
    return call;
}

Value *IrGen::gen_num_lit(const ast::NumLit *num_lit) {
    auto *constant = new Constant(num_lit->value());
    constant->set_type(IntType::get(32));
    return constant;
}

Value *IrGen::gen_unary_expr(const ast::UnaryExpr *unary_expr) {
    switch (unary_expr->op()) {
    case ast::UnaryOp::AddressOf:
        return gen_address_of(unary_expr->val());
    case ast::UnaryOp::Deref:
        return gen_deref(unary_expr->val());
    }
}

Value *IrGen::gen_var_expr(const ast::VarExpr *var_expr) {
    auto *var = m_scope_stack.peek().find_var(var_expr->name());
    assert(var != nullptr);
    return m_block->append<LoadInst>(var);
}

Value *IrGen::gen_expr(const ast::Node *expr) {
    switch (expr->kind()) {
    case ast::NodeKind::BinExpr:
        return gen_bin_expr(static_cast<const ast::BinExpr *>(expr));
    case ast::NodeKind::CallExpr:
        return gen_call_expr(static_cast<const ast::CallExpr *>(expr));
    case ast::NodeKind::NumLit:
        return gen_num_lit(static_cast<const ast::NumLit *>(expr));
    case ast::NodeKind::UnaryExpr:
        return gen_unary_expr(static_cast<const ast::UnaryExpr *>(expr));
    case ast::NodeKind::VarExpr:
        return gen_var_expr(static_cast<const ast::VarExpr *>(expr));
    default:
        assert(false);
    }
}

void IrGen::gen_decl_stmt(const ast::DeclStmt *decl_stmt) {
    assert(decl_stmt->type() != nullptr);
    // TODO: Fix IR const-correctness here.
    auto *var = m_function->append_var(const_cast<Type *>(decl_stmt->type()));
    if (decl_stmt->init_val() != nullptr) {
        auto *init_val = gen_expr(decl_stmt->init_val());
        m_block->append<StoreInst>(var, init_val);
    }
    m_scope_stack.peek().put_var(decl_stmt->name(), var);
}

void IrGen::gen_ret_stmt(const ast::RetStmt *ret_stmt) {
    auto *val = gen_expr(ret_stmt->val());
    m_block->append<RetInst>(val);
}

void IrGen::gen_stmt(const ast::Node *stmt) {
    switch (stmt->kind()) {
    case ast::NodeKind::AssignStmt:
        assert(false);
    case ast::NodeKind::CallExpr:
        gen_call_expr(static_cast<const ast::CallExpr *>(stmt));
        break;
    case ast::NodeKind::DeclStmt:
        gen_decl_stmt(static_cast<const ast::DeclStmt *>(stmt));
        break;
    case ast::NodeKind::RetStmt:
        gen_ret_stmt(static_cast<const ast::RetStmt *>(stmt));
        break;
    default:
        assert(false);
    }
}

void IrGen::gen_function_decl(const ast::FunctionDecl *function_decl) {
    // TODO: Fix IR const-correctness here.
    m_function = m_program->append_function(function_decl->name(), const_cast<Type *>(function_decl->type()));
    for (const auto *ast_arg : function_decl->args()) {
        auto *arg = m_function->append_arg();
        arg->set_name(ast_arg->name());
        arg->set_type(ast_arg->type());
    }

    if (function_decl->externed()) {
        return;
    }

    m_block = m_function->append_block();
    m_scope_stack.clear();
    m_scope_stack.emplace(/* parent */ nullptr);
    for (auto *arg : m_function->args()) {
        // TODO: Fix IR const-correctness here.
        auto *arg_var = m_function->append_var(const_cast<Type *>(arg->type()));
        m_block->append<StoreInst>(arg_var, arg);
        m_scope_stack.peek().put_var(arg->name(), arg_var);
    }

    for (const auto *stmt : function_decl->stmts()) {
        gen_stmt(stmt);
    }
}

} // namespace

std::unique_ptr<Program> gen_ir(const ast::Root *root) {
    IrGen gen;
    for (const auto *function : root->functions()) {
        gen.gen_function_decl(function);
    }
    return gen.program();
}
