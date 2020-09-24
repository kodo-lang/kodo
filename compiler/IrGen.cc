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

    Value *gen_num_lit(NumLit *);
    Value *gen_expr(AstNode *);

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

Value *IrGen::gen_num_lit(NumLit *num_lit) {
    auto *constant = new Constant(num_lit->value());
    constant->set_type(new IntType(32));
    return constant;
}

Value *IrGen::gen_expr(AstNode *expr) {
    switch (expr->kind()) {
    case NodeKind::BinExpr:
    case NodeKind::UnaryExpr:
    case NodeKind::VarExpr:
        assert(false);
    case NodeKind::NumLit:
        return gen_num_lit(static_cast<NumLit *>(expr));
    default:
        assert(false);
    }
}

void IrGen::gen_ret_stmt(RetStmt *ret_stmt) {
    auto *val = gen_expr(ret_stmt->val());
    m_block->append<RetInst>(val);
}

void IrGen::gen_stmt(AstNode *stmt) {
    switch (stmt->kind()) {
    case NodeKind::AssignStmt:
    case NodeKind::DeclStmt:
        assert(false);
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
