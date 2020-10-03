#include <IrGen.hh>

#include <ast/Nodes.hh>
#include <ir/BasicBlock.hh>
#include <ir/Constant.hh>
#include <ir/Function.hh>
#include <ir/Instructions.hh>
#include <ir/Program.hh>
#include <support/Stack.hh>

#include <fmt/color.h>
#include <fmt/core.h>

#include <algorithm>
#include <cassert>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>

namespace {

template <typename T>
class StateChanger {
    T &m_state;
    T m_old_state;

public:
    StateChanger(T &state, T new_state) : m_state(state) {
        m_old_state = state;
        m_state = new_state;
    }

    ~StateChanger() { m_state = m_old_state; }
};

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
    std::vector<std::string> m_errors;

    enum class DerefState {
        Deref,
        DontDeref,
    } m_deref_state{DerefState::Deref};

    template <typename FmtStr, typename... Args>
    void add_node_error(const ast::Node *node, const FmtStr &fmt, const Args &... args);

public:
    IrGen();

    Value *gen_address_of(const ast::Node *);
    Value *gen_deref(const ast::Node *);

    Value *gen_assign_expr(const ast::AssignExpr *);
    Value *gen_bin_expr(const ast::BinExpr *);
    Value *gen_call_expr(const ast::CallExpr *);
    Value *gen_cast_expr(const ast::CastExpr *);
    Value *gen_num_lit(const ast::NumLit *);
    Value *gen_symbol(const ast::Symbol *);
    Value *gen_unary_expr(const ast::UnaryExpr *);
    Value *gen_expr(const ast::Node *);

    void gen_decl_stmt(const ast::DeclStmt *);
    void gen_ret_stmt(const ast::RetStmt *);
    void gen_stmt(const ast::Node *);

    void gen_function_decl(const ast::FunctionDecl *);

    const std::vector<std::string> &errors() { return m_errors; }
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

template <typename FmtStr, typename... Args>
void IrGen::add_node_error(const ast::Node *node, const FmtStr &fmt, const Args &... args) {
    auto formatted = fmt::format(fmt, args...);
    auto error = fmt::format(fmt::fg(fmt::color::orange_red), "error:");
    m_errors.push_back(fmt::format("{} {} on line {}\n", error, formatted, node->line()));
}

Value *IrGen::gen_address_of(const ast::Node *expr) {
    StateChanger deref_state_changer(m_deref_state, DerefState::DontDeref);
    return gen_expr(expr);
}

Value *IrGen::gen_deref(const ast::Node *expr) {
    return m_block->append<LoadInst>(gen_expr(expr));
}

Value *IrGen::gen_assign_expr(const ast::AssignExpr *assign_expr) {
    Value *lhs = nullptr;
    {
        StateChanger deref_state_changer(m_deref_state, DerefState::DontDeref);
        lhs = gen_expr(assign_expr->lhs());
    }
    auto *rhs = gen_expr(assign_expr->rhs());
    m_block->append<StoreInst>(lhs, rhs);
    return lhs;
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
    case ast::BinOp::LessThan:
        return m_block->append<CompareInst>(CompareOp::LessThan, lhs, rhs);
    case ast::BinOp::GreaterThan:
        return m_block->append<CompareInst>(CompareOp::GreaterThan, lhs, rhs);
    default:
        assert(false);
    }
}

Value *IrGen::gen_call_expr(const ast::CallExpr *call_expr) {
    std::vector<Value *> args;
    for (const auto *ast_arg : call_expr->args()) {
        args.push_back(gen_expr(ast_arg));
    }
    auto it = std::find_if(m_program->begin(), m_program->end(), [call_expr](const Function *function) {
        return function->name() == call_expr->name();
    });
    if (it == m_program->end()) {
        add_node_error(call_expr, "no function named '{}' in current context", call_expr->name());
        return new Constant(0);
    }
    auto *call = m_block->append<CallInst>(*it, std::move(args));
    call->set_line(call_expr->line());
    return call;
}

Value *IrGen::gen_cast_expr(const ast::CastExpr *cast_expr) {
    auto *cast = m_block->append<CastInst>(CastOp::SignExtend, cast_expr->type(), gen_expr(cast_expr->val()));
    cast->set_line(cast_expr->line());
    return cast;
}

Value *IrGen::gen_num_lit(const ast::NumLit *num_lit) {
    return new Constant(num_lit->value());
}

Value *IrGen::gen_symbol(const ast::Symbol *symbol) {
    auto *var = m_scope_stack.peek().find_var(symbol->name());
    if (var == nullptr) {
        add_node_error(symbol, "no symbol named '{}' in current context", symbol->name());
        return new Constant(0);
    }
    if (m_deref_state == DerefState::DontDeref) {
        return var;
    }
    auto *load = m_block->append<LoadInst>(var);
    load->set_line(symbol->line());
    return load;
}

Value *IrGen::gen_unary_expr(const ast::UnaryExpr *unary_expr) {
    switch (unary_expr->op()) {
    case ast::UnaryOp::AddressOf:
        return gen_address_of(unary_expr->val());
    case ast::UnaryOp::Deref:
        return gen_deref(unary_expr->val());
    default:
        assert(false);
    }
}

Value *IrGen::gen_expr(const ast::Node *expr) {
    switch (expr->kind()) {
    case ast::NodeKind::BinExpr:
        return gen_bin_expr(expr->as<ast::BinExpr>());
    case ast::NodeKind::CallExpr:
        return gen_call_expr(expr->as<ast::CallExpr>());
    case ast::NodeKind::CastExpr:
        return gen_cast_expr(expr->as<ast::CastExpr>());
    case ast::NodeKind::NumLit:
        return gen_num_lit(expr->as<ast::NumLit>());
    case ast::NodeKind::Symbol:
        return gen_symbol(expr->as<ast::Symbol>());
    case ast::NodeKind::UnaryExpr:
        return gen_unary_expr(expr->as<ast::UnaryExpr>());
    default:
        assert(false);
    }
}

void IrGen::gen_decl_stmt(const ast::DeclStmt *decl_stmt) {
    assert(decl_stmt->type() != nullptr);
    auto *var = m_function->append_var(decl_stmt->type());
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
    case ast::NodeKind::AssignExpr:
        gen_assign_expr(stmt->as<ast::AssignExpr>());
        break;
    case ast::NodeKind::CallExpr:
        gen_call_expr(stmt->as<ast::CallExpr>());
        break;
    case ast::NodeKind::DeclStmt:
        gen_decl_stmt(stmt->as<ast::DeclStmt>());
        break;
    case ast::NodeKind::RetStmt:
        gen_ret_stmt(stmt->as<ast::RetStmt>());
        break;
    default:
        assert(false);
    }
}

void IrGen::gen_function_decl(const ast::FunctionDecl *function_decl) {
    m_function = m_program->append_function(function_decl->name(), function_decl->return_type());
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
        auto *arg_var = m_function->append_var(arg->type());
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
    for (const auto &error : gen.errors()) {
        fmt::print(error);
    }
    if (!gen.errors().empty()) {
        fmt::print(fmt::fg(fmt::color::orange_red), " note: Aborting due to previous errors\n");
        exit(1);
    }
    return gen.program();
}
