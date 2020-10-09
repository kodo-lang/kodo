#include <IrGen.hh>

#include <Error.hh>
#include <ast/Nodes.hh>
#include <ir/BasicBlock.hh>
#include <ir/Constants.hh>
#include <ir/Function.hh>
#include <ir/Instructions.hh>
#include <ir/Program.hh>
#include <support/Assert.hh>
#include <support/Stack.hh>

#include <algorithm>
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
    std::unordered_map<std::string_view, ir::Value *> m_vars;

public:
    explicit Scope(const Scope *parent) : m_parent(parent) {}

    ir::Value *find_var(std::string_view name);
    void put_var(std::string_view name, ir::Value *value);
};

class IrGen {
    std::unique_ptr<ir::Program> m_program;
    ir::Function *m_function{nullptr};
    ir::BasicBlock *m_block{nullptr};
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

    ir::Value *gen_address_of(const ast::Node *);
    ir::Value *gen_deref(const ast::Node *);

    ir::Value *gen_assign_expr(const ast::AssignExpr *);
    ir::Value *gen_bin_expr(const ast::BinExpr *);
    ir::Value *gen_call_expr(const ast::CallExpr *);
    ir::Value *gen_cast_expr(const ast::CastExpr *);
    ir::Value *gen_num_lit(const ast::NumLit *);
    ir::Value *gen_string_lit(const ast::StringLit *);
    ir::Value *gen_symbol(const ast::Symbol *);
    ir::Value *gen_unary_expr(const ast::UnaryExpr *);
    ir::Value *gen_expr(const ast::Node *);

    void gen_decl_stmt(const ast::DeclStmt *);
    void gen_if_stmt(const ast::IfStmt *);
    void gen_ret_stmt(const ast::RetStmt *);
    void gen_stmt(const ast::Node *);

    void gen_block(const ast::Block *);
    void gen_function_decl(const ast::FunctionDecl *);

    const std::vector<std::string> &errors() { return m_errors; }
    std::unique_ptr<ir::Program> program() { return std::move(m_program); }
};

ir::Value *Scope::find_var(std::string_view name) {
    for (const auto *scope = this; scope != nullptr; scope = scope->m_parent) {
        if (scope->m_vars.contains(name)) {
            return scope->m_vars.at(name);
        }
    }
    return nullptr;
}

void Scope::put_var(std::string_view name, ir::Value *value) {
    m_vars.emplace(name, value);
}

IrGen::IrGen() {
    m_program = std::make_unique<ir::Program>();
}

template <typename FmtStr, typename... Args>
void IrGen::add_node_error(const ast::Node *node, const FmtStr &fmt, const Args &... args) {
    m_errors.push_back(format_error(node, fmt, args...));
}

ir::Value *IrGen::gen_address_of(const ast::Node *expr) {
    StateChanger deref_state_changer(m_deref_state, DerefState::DontDeref);
    return gen_expr(expr);
}

ir::Value *IrGen::gen_deref(const ast::Node *expr) {
    return m_block->append<ir::LoadInst>(gen_expr(expr));
}

ir::Value *IrGen::gen_assign_expr(const ast::AssignExpr *assign_expr) {
    ir::Value *lhs = nullptr;
    {
        StateChanger deref_state_changer(m_deref_state, DerefState::DontDeref);
        lhs = gen_expr(assign_expr->lhs());
    }
    auto *rhs = gen_expr(assign_expr->rhs());
    auto *store = m_block->append<ir::StoreInst>(lhs, rhs);
    store->set_line(assign_expr->line());
    return lhs;
}

ir::Value *IrGen::gen_bin_expr(const ast::BinExpr *bin_expr) {
    auto *lhs = gen_expr(bin_expr->lhs());
    auto *rhs = gen_expr(bin_expr->rhs());
    switch (bin_expr->op()) {
    case ast::BinOp::Add:
        return m_block->append<ir::BinaryInst>(ir::BinaryOp::Add, lhs, rhs);
    case ast::BinOp::Sub:
        return m_block->append<ir::BinaryInst>(ir::BinaryOp::Sub, lhs, rhs);
    case ast::BinOp::Mul:
        return m_block->append<ir::BinaryInst>(ir::BinaryOp::Mul, lhs, rhs);
    case ast::BinOp::Div:
        return m_block->append<ir::BinaryInst>(ir::BinaryOp::Div, lhs, rhs);
    case ast::BinOp::LessThan:
        return m_block->append<ir::CompareInst>(ir::CompareOp::LessThan, lhs, rhs);
    case ast::BinOp::GreaterThan:
        return m_block->append<ir::CompareInst>(ir::CompareOp::GreaterThan, lhs, rhs);
    default:
        ENSURE_NOT_REACHED();
    }
}

ir::Value *IrGen::gen_call_expr(const ast::CallExpr *call_expr) {
    std::vector<ir::Value *> args;
    for (const auto *ast_arg : call_expr->args()) {
        args.push_back(gen_expr(ast_arg));
    }
    auto it = std::find_if(m_program->begin(), m_program->end(), [call_expr](const ir::Function *function) {
        return function->name() == call_expr->name();
    });
    if (it == m_program->end()) {
        add_node_error(call_expr, "no function named '{}' in current context", call_expr->name());
        return ir::ConstantNull::get();
    }
    return m_block->append<ir::CallInst>(*it, std::move(args));
}

ir::Value *IrGen::gen_cast_expr(const ast::CastExpr *cast_expr) {
    // TODO: Always setting this to SignExtend is a bit misleading.
    auto *expr = gen_expr(cast_expr->val());
    return m_block->append<ir::CastInst>(ir::CastOp::SignExtend, cast_expr->type(), expr);
}

ir::Value *IrGen::gen_num_lit(const ast::NumLit *num_lit) {
    // TODO: Work out best fit type based on constant value.
    return ir::ConstantInt::get(InvalidType::get(), num_lit->value());
}

ir::Value *IrGen::gen_string_lit(const ast::StringLit *string_lit) {
    // TODO: Avoid copying string here?
    return ir::ConstantString::get(string_lit->value());
}

ir::Value *IrGen::gen_symbol(const ast::Symbol *symbol) {
    auto *var = m_scope_stack.peek().find_var(symbol->name());
    if (var == nullptr) {
        add_node_error(symbol, "no symbol named '{}' in current context", symbol->name());
        return ir::ConstantNull::get();
    }
    if (m_deref_state == DerefState::DontDeref) {
        return var;
    }
    return m_block->append<ir::LoadInst>(var);
}

ir::Value *IrGen::gen_unary_expr(const ast::UnaryExpr *unary_expr) {
    switch (unary_expr->op()) {
    case ast::UnaryOp::AddressOf:
        return gen_address_of(unary_expr->val());
    case ast::UnaryOp::Deref:
        return gen_deref(unary_expr->val());
    default:
        ENSURE_NOT_REACHED();
    }
}

ir::Value *IrGen::gen_expr(const ast::Node *expr) {
    auto *value = [this, expr]() {
      switch (expr->kind()) {
      case ast::NodeKind::AssignExpr:
          return gen_assign_expr(expr->as<ast::AssignExpr>());
      case ast::NodeKind::BinExpr:
          return gen_bin_expr(expr->as<ast::BinExpr>());
      case ast::NodeKind::CallExpr:
          return gen_call_expr(expr->as<ast::CallExpr>());
      case ast::NodeKind::CastExpr:
          return gen_cast_expr(expr->as<ast::CastExpr>());
      case ast::NodeKind::NumLit:
          return gen_num_lit(expr->as<ast::NumLit>());
      case ast::NodeKind::StringLit:
          return gen_string_lit(expr->as<ast::StringLit>());
      case ast::NodeKind::Symbol:
          return gen_symbol(expr->as<ast::Symbol>());
      case ast::NodeKind::UnaryExpr:
          return gen_unary_expr(expr->as<ast::UnaryExpr>());
      default:
          ENSURE_NOT_REACHED();
      }
    }();
    if (auto *inst = value->as_or_null<ir::Instruction>()) {
        inst->set_line(expr->line());
    }
    return value;
}

void IrGen::gen_decl_stmt(const ast::DeclStmt *decl_stmt) {
    if (m_scope_stack.peek().find_var(decl_stmt->name()) != nullptr) {
        add_node_error(decl_stmt, "redeclaration of variable '{}'", decl_stmt->name());
        return;
    }

    ASSERT(decl_stmt->type() != nullptr);
    auto *var = m_function->append_var(decl_stmt->type(), decl_stmt->is_mutable());
    var->set_name(decl_stmt->name());
    if (decl_stmt->init_val() != nullptr) {
        auto *init_val = gen_expr(decl_stmt->init_val());
        auto *store = m_block->append<ir::StoreInst>(var, init_val);
        store->set_line(decl_stmt->line());
    }
    m_scope_stack.peek().put_var(decl_stmt->name(), var);
}

void IrGen::gen_if_stmt(const ast::IfStmt *if_stmt) {
    auto *cond = gen_expr(if_stmt->expr());
    auto *true_dst = m_function->append_block();
    auto *false_dst = m_function->append_block();
    m_block->append<ir::CondBranchInst>(cond, true_dst, false_dst);
    m_block = true_dst;
    gen_block(if_stmt->block());
    if ((m_block->begin() == m_block->end()) || m_block->terminator()->inst_kind() != ir::InstKind::Ret) {
        m_block->append<ir::BranchInst>(false_dst);
    }
    m_block = false_dst;
}

void IrGen::gen_ret_stmt(const ast::RetStmt *ret_stmt) {
    auto *val = gen_expr(ret_stmt->val());
    m_block->append<ir::RetInst>(val);
}

void IrGen::gen_stmt(const ast::Node *stmt) {
    switch (stmt->kind()) {
    case ast::NodeKind::DeclStmt:
        gen_decl_stmt(stmt->as<ast::DeclStmt>());
        break;
    case ast::NodeKind::IfStmt:
        gen_if_stmt(stmt->as<ast::IfStmt>());
        break;
    case ast::NodeKind::RetStmt:
        gen_ret_stmt(stmt->as<ast::RetStmt>());
        break;
    default:
        gen_expr(stmt);
    }
}

void IrGen::gen_block(const ast::Block *block) {
    m_scope_stack.emplace(m_scope_stack.peek());
    for (const auto *stmt : block->stmts()) {
        gen_stmt(stmt);
    }
    m_scope_stack.pop();
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
        // TODO: let and var syntax for function args.
        auto *arg_var = m_function->append_var(arg->type(), true);
        m_block->append<ir::StoreInst>(arg_var, arg);
        m_scope_stack.peek().put_var(arg->name(), arg_var);
    }

    ASSERT(function_decl->block() != nullptr);
    gen_block(function_decl->block());
}

} // namespace

std::unique_ptr<ir::Program> gen_ir(const ast::Root *root) {
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
