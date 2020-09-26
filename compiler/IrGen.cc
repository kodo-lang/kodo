#include <IrGen.hh>

#include <Ast.hh>
#include <Stack.hh>
#include <ir/BasicBlock.hh>
#include <ir/Constant.hh>
#include <ir/Function.hh>
#include <ir/Instructions.hh>
#include <ir/Program.hh>

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

    Value *gen_address_of(AstNode *);
    Value *gen_deref(AstNode *);

    Value *gen_num_lit(NumLit *);
    Value *gen_unary_expr(UnaryExpr *);
    Value *gen_var_expr(VarExpr *);
    Value *gen_expr(AstNode *);

    void gen_decl_stmt(DeclStmt *);
    void gen_ret_stmt(RetStmt *);
    void gen_stmt(AstNode *);

    void gen_function_decl(FunctionDecl *);

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

Value *IrGen::gen_address_of(AstNode *expr) {
    assert(expr->kind() == NodeKind::VarExpr);
    auto *var_expr = static_cast<VarExpr *>(expr);
    auto *var = m_scope_stack.peek().find_var(var_expr->name());
    assert(var != nullptr);
    return var;
}

Value *IrGen::gen_deref(AstNode *expr) {
    return m_block->append<LoadInst>(gen_expr(expr));
}

Value *IrGen::gen_num_lit(NumLit *num_lit) {
    auto *constant = new Constant(num_lit->value());
    constant->set_type(new IntType(32));
    return constant;
}

Value *IrGen::gen_unary_expr(UnaryExpr *unary_expr) {
    switch (unary_expr->op()) {
    case UnaryOp::AddressOf:
        return gen_address_of(unary_expr->val());
    case UnaryOp::Deref:
        return gen_deref(unary_expr->val());
    }
}

Value *IrGen::gen_var_expr(VarExpr *var_expr) {
    auto *var = m_scope_stack.peek().find_var(var_expr->name());
    assert(var != nullptr);
    return m_block->append<LoadInst>(var);
}

Value *IrGen::gen_expr(AstNode *expr) {
    switch (expr->kind()) {
    case NodeKind::BinExpr:
        assert(false);
    case NodeKind::NumLit:
        return gen_num_lit(static_cast<NumLit *>(expr));
    case NodeKind::UnaryExpr:
        return gen_unary_expr(static_cast<UnaryExpr *>(expr));
    case NodeKind::VarExpr:
        return gen_var_expr(static_cast<VarExpr *>(expr));
    default:
        assert(false);
    }
}

void IrGen::gen_decl_stmt(DeclStmt *decl_stmt) {
    assert(decl_stmt->type() != nullptr);
    auto *var = m_function->append_var(decl_stmt->type());
    m_scope_stack.peek().put_var(decl_stmt->name(), var);
    if (decl_stmt->init_val() != nullptr) {
        auto *init_val = gen_expr(decl_stmt->init_val());
        m_block->append<StoreInst>(var, init_val);
    }
}

void IrGen::gen_ret_stmt(RetStmt *ret_stmt) {
    auto *val = gen_expr(ret_stmt->val());
    m_block->append<RetInst>(val);
}

void IrGen::gen_stmt(AstNode *stmt) {
    switch (stmt->kind()) {
    case NodeKind::AssignStmt:
        assert(false);
    case NodeKind::DeclStmt:
        gen_decl_stmt(static_cast<DeclStmt *>(stmt));
        break;
    case NodeKind::RetStmt:
        gen_ret_stmt(static_cast<RetStmt *>(stmt));
        break;
    default:
        assert(false);
    }
}

void IrGen::gen_function_decl(FunctionDecl *function_decl) {
    m_function = m_program->append_function(function_decl->name());
    m_block = m_function->append_block();
    for (auto *ast_arg : function_decl->args()) {
        auto *arg = m_function->append_arg();
        arg->set_name(ast_arg->name());
        arg->set_type(ast_arg->type());
    }

    m_scope_stack.clear();
    m_scope_stack.emplace(/* parent */ nullptr);
    for (auto *stmt : function_decl->stmts()) {
        gen_stmt(stmt);
    }
}

} // namespace

std::unique_ptr<Program> gen_ir(AstNode *ast) {
    assert(ast->kind() == NodeKind::FunctionDecl);
    IrGen gen;
    gen.gen_function_decl(static_cast<FunctionDecl *>(ast));
    return gen.program();
}
