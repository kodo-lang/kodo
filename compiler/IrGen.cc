#include <IrGen.hh>

#include <ast/Nodes.hh>
#include <ir/BasicBlock.hh>
#include <ir/Constants.hh>
#include <ir/Function.hh>
#include <ir/Instructions.hh>
#include <ir/Program.hh>
#include <ir/Types.hh>
#include <support/Assert.hh>
#include <support/Error.hh>
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
    std::unordered_map<std::string, const ir::Type *> m_types;
    std::unordered_map<const ir::Type *, const std::string *> m_types_reverse;
    std::unordered_map<std::string_view, ir::Value *> m_vars;

public:
    explicit Scope(const Scope *parent) : m_parent(parent) {}

    const ir::Type *find_type(const std::string &name);
    const std::string &find_type_reverse(const ir::Type *type);
    void put_type(std::string name, const ir::Type *type);

    ir::Value *find_var(std::string_view name);
    void put_var(std::string_view name, ir::Value *value);
};

class IrGen {
    std::unique_ptr<ir::Program> m_program;
    ir::Function *m_function{nullptr};
    ir::BasicBlock *m_block{nullptr};
    Stack<Scope> m_scope_stack;

    enum class DerefState {
        Deref,
        DontDeref,
    } m_deref_state{DerefState::Deref};

    enum class MemberLoadState {
        DontLoad,
        Load,
    } m_member_load_state{MemberLoadState::Load};

public:
    IrGen();

    ir::Value *create_call(const ast::CallExpr *, ir::Function *, ir::Value *);
    void create_store(const ast::Node *, ir::Value *, ir::Value *);
    ir::Function *find_function(const std::string &);
    ir::Function *find_function(const ast::Symbol *);
    ir::Function *find_or_create_function(const ast::Symbol *, const ir::Type *);
    ir::Value *get_member_ptr(ir::Value *, int);
    const ir::Type *get_type(const ast::Node *, const std::string &);

    const ir::Type *gen_base_type(const ast::Symbol *);
    const ir::Type *gen_pointer_type(const ast::PointerType *);
    const ir::Type *gen_struct_type(const ast::StructType *);
    const ir::Type *gen_type(const ast::Node *);

    ir::Value *gen_address_of(const ast::Node *);
    ir::Value *gen_deref(const ast::Node *);

    ir::Value *gen_asm_expr(const ast::AsmExpr *);
    ir::Value *gen_assign_expr(const ast::AssignExpr *);
    ir::Value *gen_bin_expr(const ast::BinExpr *);
    ir::Value *gen_call_expr(const ast::CallExpr *);
    ir::Value *gen_cast_expr(const ast::CastExpr *);
    ir::Value *gen_construct_expr(const ast::ConstructExpr *);
    ir::Value *gen_member_expr(const ast::MemberExpr *);
    ir::Value *gen_num_lit(const ast::NumLit *);
    ir::Value *gen_string_lit(const ast::StringLit *);
    ir::Value *gen_symbol(const ast::Symbol *);
    ir::Value *gen_unary_expr(const ast::UnaryExpr *);
    ir::Value *gen_expr_value(const ast::Node *);
    ir::Value *gen_expr(const ast::Node *);

    void gen_decl_stmt(const ast::DeclStmt *);
    void gen_if_stmt(const ast::IfStmt *);
    void gen_ret_stmt(const ast::RetStmt *);
    void gen_stmt(const ast::Node *);

    void gen_block(const ast::Block *);
    void gen_function_decl(const ast::FunctionDecl *);
    void gen_type_decl(const ast::TypeDecl *);
    void gen_decl(const ast::Node *);

    std::unique_ptr<ir::Program> program() { return std::move(m_program); }
};

const ir::Type *Scope::find_type(const std::string &name) {
    for (const auto *scope = this; scope != nullptr; scope = scope->m_parent) {
        if (scope->m_types.contains(name)) {
            return scope->m_types.at(name);
        }
    }
    return nullptr;
}

const std::string &Scope::find_type_reverse(const ir::Type *type) {
    for (const auto *scope = this; scope != nullptr; scope = scope->m_parent) {
        if (scope->m_types_reverse.contains(type)) {
            return *scope->m_types_reverse.at(type);
        }
    }
    ENSURE_NOT_REACHED();
}

void Scope::put_type(std::string name, const ir::Type *type) {
    const auto *moved_name = &m_types.emplace(std::move(name), type).first->first;
    m_types_reverse.emplace(type, moved_name);
}

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
    m_scope_stack.emplace(/* parent */ nullptr);
}

ir::Value *IrGen::create_call(const ast::CallExpr *call_expr, ir::Function *callee, ir::Value *this_arg) {
    std::vector<ir::Value *> args;
    if (this_arg != nullptr) {
        args.push_back(this_arg);
    }
    for (const auto *ast_arg : call_expr->args()) {
        args.push_back(gen_expr(ast_arg));
    }
    if (callee == nullptr) {
        print_error(call_expr, "no function named '{}' in current context", call_expr->name()->parts()[0]);
        return ir::ConstantNull::get(m_program.get());
    }
    return m_block->append<ir::CallInst>(callee, std::move(args));
}

void IrGen::create_store(const ast::Node *node, ir::Value *ptr, ir::Value *val) {
    auto *store = m_block->append<ir::StoreInst>(ptr, val);
    store->set_line(node->line());
}

std::string mangle(const ast::Symbol *name) {
    std::string mangled_name;
    for (bool first = true; const auto &part : name->parts()) {
        if (!first) {
            mangled_name += '_';
        }
        first = false;
        mangled_name += part;
    }
    return std::move(mangled_name);
}

ir::Function *IrGen::find_function(const std::string &name) {
    for (auto *function : *m_program) {
        if (function->name() == name) {
            return function;
        }
    }
    return nullptr;
}

ir::Function *IrGen::find_function(const ast::Symbol *name) {
    return find_function(mangle(name));
}

ir::Function *IrGen::find_or_create_function(const ast::Symbol *name, const ir::Type *return_type) {
    if (auto *function = find_function(name)) {
        ASSERT(function->return_type() == return_type);
        return function;
    }
    return m_program->append_function(mangle(name), return_type);
}

ir::Value *IrGen::get_member_ptr(ir::Value *ptr, int index) {
    std::vector<ir::Value *> indices;
    indices.reserve(2);
    indices.push_back(ir::ConstantInt::get(m_program->int_type(32, false), 0));
    indices.push_back(ir::ConstantInt::get(m_program->int_type(32, false), index));
    return m_block->append<ir::LeaInst>(ptr, std::move(indices));
}

const ir::Type *IrGen::get_type(const ast::Node *node, const std::string &name) {
    if (const auto *type = m_scope_stack.peek().find_type(name)) {
        return type;
    }
    print_error(node, "no type named '{}' in current context", name);
    return m_program->invalid_type();
}

const ir::Type *IrGen::gen_base_type(const ast::Symbol *symbol) {
    ASSERT(symbol->parts().size() == 1);
    const auto &base = symbol->parts()[0];
    if (base == "bool") {
        return m_program->bool_type();
    }
    if (base == "void") {
        return m_program->void_type();
    }
    if (base.starts_with('i') || base.starts_with('u')) {
        auto bit_width_str = base.substr(1);
        if (bit_width_str.find_first_not_of("0123456789") == std::string::npos) {
            int bit_width = std::stoi(bit_width_str);
            return m_program->int_type(bit_width, base.starts_with('i'));
        }
    }
    return get_type(symbol, base);
}

const ir::Type *IrGen::gen_pointer_type(const ast::PointerType *pointer_type) {
    const auto *pointee_type = gen_type(pointer_type->pointee_type());
    return m_program->pointer_type(pointee_type, pointer_type->is_mutable());
}

const ir::Type *IrGen::gen_struct_type(const ast::StructType *struct_type) {
    std::vector<ir::StructField> fields;
    for (const auto *struct_field : struct_type->fields()) {
        fields.emplace_back(struct_field->name(), gen_type(struct_field->type()));
    }
    return m_program->struct_type(std::move(fields));
}

const ir::Type *IrGen::gen_type(const ast::Node *node) {
    if (node == nullptr) {
        return m_program->invalid_type();
    }
    switch (node->kind()) {
    case ast::NodeKind::Symbol:
        return gen_base_type(node->as<ast::Symbol>());
    case ast::NodeKind::PointerType:
        return gen_pointer_type(node->as<ast::PointerType>());
    case ast::NodeKind::StructType:
        return gen_struct_type(node->as<ast::StructType>());
    default:
        ENSURE_NOT_REACHED();
    }
}

ir::Value *IrGen::gen_address_of(const ast::Node *expr) {
    StateChanger deref_state_changer(m_deref_state, DerefState::DontDeref);
    return gen_expr(expr);
}

ir::Value *IrGen::gen_deref(const ast::Node *expr) {
    return m_block->append<ir::LoadInst>(gen_expr(expr));
}

ir::Value *IrGen::gen_asm_expr(const ast::AsmExpr *asm_expr) {
    // TODO: Avoid copying.
    auto clobbers = asm_expr->clobbers();
    std::vector<std::pair<std::string, ir::Value *>> inputs;
    std::vector<std::pair<std::string, ir::Value *>> outputs;
    inputs.reserve(asm_expr->inputs().size());
    outputs.reserve(asm_expr->outputs().size());
    for (const auto &[input, expr] : asm_expr->inputs()) {
        inputs.emplace_back(input, gen_expr(expr.get()));
    }
    for (const auto &[output, expr] : asm_expr->outputs()) {
        StateChanger deref_state_changer(m_deref_state, DerefState::DontDeref);
        outputs.emplace_back(output, gen_expr(expr.get()));
    }
    auto *inline_asm = m_block->append<ir::InlineAsmInst>(asm_expr->instruction(), std::move(clobbers),
                                                          std::move(inputs), std::move(outputs));
    inline_asm->set_type(m_program->void_type());
    return inline_asm;
}

ir::Value *IrGen::gen_assign_expr(const ast::AssignExpr *assign_expr) {
    ir::Value *lhs = nullptr;
    {
        StateChanger deref_state_changer(m_deref_state, DerefState::DontDeref);
        StateChanger member_load_state_changer(m_member_load_state, MemberLoadState::DontLoad);
        lhs = gen_expr(assign_expr->lhs());
    }
    auto *rhs = gen_expr(assign_expr->rhs());
    create_store(assign_expr, lhs, rhs);
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
    auto *callee = find_function(call_expr->name());
    return create_call(call_expr, callee, nullptr);
}

ir::Value *IrGen::gen_cast_expr(const ast::CastExpr *cast_expr) {
    // TODO: Always setting this to SignExtend is a bit misleading.
    auto *expr = gen_expr(cast_expr->val());
    const auto *type = gen_type(cast_expr->type());
    return m_block->append<ir::CastInst>(ir::CastOp::SignExtend, type, expr);
}

ir::Value *IrGen::gen_construct_expr(const ast::ConstructExpr *construct_expr) {
    const auto *type = get_type(construct_expr, construct_expr->name())->as<ir::StructType>();
    ASSERT(construct_expr->args().size() == type->fields().size());
    auto *tmp_var = m_function->append_var(type, true);
    for (int i = 0; const auto *arg : construct_expr->args()) {
        auto *lea = get_member_ptr(tmp_var, i);
        lea->set_type(m_program->pointer_type(type->fields()[i++].type(), true));
        create_store(construct_expr, lea, gen_expr(arg));
    }
    if (m_deref_state == DerefState::DontDeref) {
        return tmp_var;
    }
    return m_block->append<ir::LoadInst>(tmp_var);
}

ir::Value *IrGen::gen_member_expr(const ast::MemberExpr *member_expr) {
    ir::Value *lhs = nullptr;
    {
        StateChanger deref_state_changer(m_deref_state, DerefState::DontDeref);
        StateChanger member_load_state_changer(m_member_load_state, MemberLoadState::DontLoad);
        lhs = gen_expr(member_expr->lhs());
    }
    const auto *type = lhs->type();
    if (auto *var = lhs->as_or_null<ir::LocalVar>()) {
        type = var->var_type();
    }
    if (const auto *ptr_type = type->as_or_null<ir::PointerType>()) {
        type = ptr_type->pointee_type();
    }
    const auto *struct_type = type->as<ir::StructType>();
    if (member_expr->is_pointer()) {
        lhs = m_block->append<ir::LoadInst>(lhs);
    }
    if (const auto *call_expr = member_expr->rhs()->as_or_null<ast::CallExpr>()) {
        auto *callee =
            find_function(m_scope_stack.peek().find_type_reverse(struct_type) + '_' + call_expr->name()->parts()[0]);
        return create_call(call_expr, callee, lhs);
    }
    const auto *rhs = member_expr->rhs()->as<ast::Symbol>();
    ASSERT(rhs->parts().size() == 1);
    const auto &rhs_name = rhs->parts()[0];
    auto it = std::find_if(struct_type->fields().begin(), struct_type->fields().end(),
                           [&rhs_name](const ir::StructField &field) {
                               return field.name() == rhs_name;
                           });
    if (it == struct_type->fields().end()) {
        const auto &struct_type_name = m_scope_stack.peek().find_type_reverse(struct_type);
        print_error(member_expr, "struct '{}' has no member named '{}'", struct_type_name, rhs_name);
        return ir::ConstantNull::get(m_program.get());
    }
    int index = std::distance(struct_type->fields().begin(), it);
    auto *lea = get_member_ptr(lhs, index);
    lea->set_type(m_program->pointer_type(it->type(), true));
    if (m_member_load_state == MemberLoadState::Load) {
        return m_block->append<ir::LoadInst>(lea);
    }
    return lea;
}

ir::Value *IrGen::gen_num_lit(const ast::NumLit *num_lit) {
    // `+ 1` for signed bit.
    int bit_width = static_cast<int>(std::ceil(std::log2(std::max(1UL, num_lit->value())))) + 1;
    return ir::ConstantInt::get(m_program->int_type(bit_width, true), num_lit->value());
}

ir::Value *IrGen::gen_string_lit(const ast::StringLit *string_lit) {
    // TODO: Avoid copying string here?
    return ir::ConstantString::get(m_program.get(), string_lit->value());
}

ir::Value *IrGen::gen_symbol(const ast::Symbol *symbol) {
    ASSERT(symbol->parts().size() == 1);
    const auto &name = symbol->parts()[0];
    auto *var = m_scope_stack.peek().find_var(name);
    if (var == nullptr) {
        print_error(symbol, "no symbol named '{}' in current context", name);
        return ir::ConstantNull::get(m_program.get());
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

ir::Value *IrGen::gen_expr_value(const ast::Node *expr) {
    switch (expr->kind()) {
    case ast::NodeKind::AsmExpr:
        return gen_asm_expr(expr->as<ast::AsmExpr>());
    case ast::NodeKind::AssignExpr:
        return gen_assign_expr(expr->as<ast::AssignExpr>());
    case ast::NodeKind::BinExpr:
        return gen_bin_expr(expr->as<ast::BinExpr>());
    case ast::NodeKind::CallExpr:
        return gen_call_expr(expr->as<ast::CallExpr>());
    case ast::NodeKind::CastExpr:
        return gen_cast_expr(expr->as<ast::CastExpr>());
    case ast::NodeKind::ConstructExpr:
        return gen_construct_expr(expr->as<ast::ConstructExpr>());
    case ast::NodeKind::MemberExpr:
        return gen_member_expr(expr->as<ast::MemberExpr>());
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
}

ir::Value *IrGen::gen_expr(const ast::Node *expr) {
    auto *value = gen_expr_value(expr);
    if (auto *inst = value->as_or_null<ir::Instruction>()) {
        inst->set_line(expr->line());
    }
    return value;
}

void IrGen::gen_decl_stmt(const ast::DeclStmt *decl_stmt) {
    if (m_scope_stack.peek().find_var(decl_stmt->name()) != nullptr) {
        print_error(decl_stmt, "redeclaration of variable '{}'", decl_stmt->name());
        return;
    }
    const auto *type = gen_type(decl_stmt->type());
    auto *var = m_function->append_var(type, decl_stmt->is_mutable());
    var->set_name(decl_stmt->name());
    if (decl_stmt->init_val() != nullptr) {
        auto *init_val = gen_expr(decl_stmt->init_val());
        create_store(decl_stmt, var, init_val);
        if (type->is<ir::InvalidType>()) {
            var->set_var_type(init_val->type());
        }
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
    if ((m_block->begin() == m_block->end()) || m_block->terminator()->kind() != ir::InstKind::Ret) {
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
    const auto *return_type = gen_type(function_decl->return_type());
    m_function = find_or_create_function(function_decl->name(), return_type);
    if (function_decl->instance()) {
        ASSERT(function_decl->name()->parts().size() == 2);
        const auto *container_type = get_type(function_decl, function_decl->name()->parts()[0])->as<ir::StructType>();
        auto *this_arg = m_function->append_arg(false);
        this_arg->set_name("this");
        this_arg->set_type(m_program->pointer_type(container_type, false));
    }
    for (const auto *ast_arg : function_decl->args()) {
        auto *arg = m_function->append_arg(ast_arg->is_mutable());
        arg->set_name(ast_arg->name());
        arg->set_type(gen_type(ast_arg->type()));
    }

    if (function_decl->externed()) {
        ASSERT(!function_decl->instance());
        return;
    }

    m_block = m_function->append_block();
    m_scope_stack.emplace(m_scope_stack.peek());
    for (auto *arg : m_function->args()) {
        // TODO: This is a very lazy (clang-inspired) approach to arguments. We can probably do some optimisation in
        //       IrGen and get extra performance by not relying on stack slot promotion.
        auto *arg_var = m_function->append_var(arg->type(), arg->is_mutable());
        arg_var->set_name(arg->name());
        m_block->append<ir::StoreInst>(arg_var, arg);
        m_scope_stack.peek().put_var(arg->name(), arg_var);
    }

    ASSERT(function_decl->block() != nullptr);
    gen_block(function_decl->block());
    m_scope_stack.pop();

    // Insert implicit return if needed.
    auto *return_block = *(--m_function->end());
    if (m_function->return_type()->is<ir::VoidType>() && return_block->terminator()->kind() != ir::InstKind::Ret) {
        // TODO: Special return void instruction?
        return_block->append<ir::RetInst>(nullptr);
    }
}

void IrGen::gen_type_decl(const ast::TypeDecl *type_decl) {
    const auto *type = gen_type(type_decl->type());
    m_scope_stack.peek().put_type(type_decl->name(), type);
}

void IrGen::gen_decl(const ast::Node *decl) {
    switch (decl->kind()) {
    case ast::NodeKind::FunctionDecl:
        gen_function_decl(decl->as<ast::FunctionDecl>());
        break;
    case ast::NodeKind::ImportStmt:
        // Handled in Compiler.
        break;
    case ast::NodeKind::TypeDecl:
        gen_type_decl(decl->as<ast::TypeDecl>());
        break;
    default:
        ENSURE_NOT_REACHED();
    }
}

} // namespace

std::unique_ptr<ir::Program> gen_ir(std::vector<std::unique_ptr<ast::Root>> &&roots) {
    IrGen gen;
    for (auto &root : roots) {
        for (const auto *decl : root->decls()) {
            gen.gen_decl(decl);
        }
    }
    return gen.program();
}
