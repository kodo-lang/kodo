#include <IrGen.hh>

#include <ast/Nodes.hh>
#include <ir/BasicBlock.hh>
#include <ir/Constants.hh>
#include <ir/Function.hh>
#include <ir/Instructions.hh>
#include <ir/Program.hh>
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
    std::unordered_map<std::string_view, ir::Value *> m_vars;

public:
    explicit Scope(const Scope *parent) : m_parent(parent) {}

    const ir::Type *find_type(const std::string &name);
    void put_type(std::string name, const ir::Type *type);

    ir::Value *find_var(std::string_view name);
    void put_var(std::string_view name, ir::Value *value);
};

class IrGen {
    std::unique_ptr<ir::Program> m_program;
    ir::Function *m_function{nullptr};
    ir::BasicBlock *m_block{nullptr};
    Stack<Scope> m_scope_stack;
    std::unordered_map<const ir::StructType *, const std::vector<ast::StructField> &> m_struct_types;

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

    void create_store(const ast::Node *, ir::Value *, ir::Value *);
    ir::Value *get_member_ptr(ir::Value *, int);

    const ir::Type *gen_base_type(const ast::Node *, const std::string &);
    const ir::Type *gen_pointer_type(const ast::Node *, const ast::Type &, bool is_mutable);
    const ir::Type *gen_struct_type(const ast::Node *, const std::vector<ast::StructField> &);
    const ir::Type *gen_type(const ast::Node *, const ast::Type &);

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

void Scope::put_type(std::string name, const ir::Type *type) {
    m_types.emplace(std::move(name), type);
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

void IrGen::create_store(const ast::Node *node, ir::Value *ptr, ir::Value *val) {
    // Generate memcpy for structs.
    // TODO: Re-enable this.
    if (false && val->type()->is<ir::StructType>()) {
        auto *len = ir::ConstantInt::get(ir::IntType::get_unsigned(64), val->type()->size_in_bytes());
        auto *copy = m_block->append<ir::CopyInst>(ptr, val, len);
        copy->set_line(node->line());
        return;
    }
    const auto *ptr_type = ptr->type()->as<ir::PointerType>();
    if (const auto *type = val->type()->as_or_null<ir::StructType>()) {
        const auto *constant_val = val->as_or_null<ir::Constant>();
        const auto *struct_val = constant_val != nullptr ? constant_val->as_or_null<ir::ConstantStruct>() : nullptr;
        if (struct_val != nullptr) {
            bool is_mutable = ptr_type->is_mutable();
            is_mutable |= ptr->is<ir::LocalVar>();
            for (int i = 0; i < type->fields().size(); i++) {
                auto *member_ptr = get_member_ptr(ptr, i);
                member_ptr->set_type(ir::PointerType::get(type->fields()[i], is_mutable));
                // Break up nested structs.
                if (type->fields()[i]->is<ir::StructType>()) {
                    create_store(node, member_ptr, struct_val->elems()[i]);
                    continue;
                }
                m_block->append<ir::StoreInst>(member_ptr, struct_val->elems()[i]);
            }
            return;
        }
    }
    auto *store = m_block->append<ir::StoreInst>(ptr, val);
    store->set_line(node->line());
}

ir::Value *IrGen::get_member_ptr(ir::Value *ptr, int index) {
    std::vector<ir::Value *> indices;
    indices.reserve(2);
    indices.push_back(ir::ConstantInt::get(ir::IntType::get_unsigned(32), 0));
    indices.push_back(ir::ConstantInt::get(ir::IntType::get_unsigned(32), index));
    return m_block->append<ir::LeaInst>(ptr, std::move(indices));
}

const ir::Type *IrGen::gen_base_type(const ast::Node *node, const std::string &base) {
    if (base == "bool") {
        return ir::BoolType::get();
    }
    if (base == "void") {
        return ir::VoidType::get();
    }
    if (base.starts_with('i') || base.starts_with('u')) {
        auto bit_width_str = base.substr(1);
        if (bit_width_str.find_first_not_of("0123456789") == std::string::npos) {
            int bit_width = std::stoi(bit_width_str);
            return ir::IntType::get(bit_width, base.starts_with('i'));
        }
    }
    if (const auto *type = m_scope_stack.peek().find_type(base)) {
        return type;
    }
    print_error(node, "invalid type '{}'", base);
    return ir::InvalidType::get();
}

const ir::Type *IrGen::gen_pointer_type(const ast::Node *node, const ast::Type &ast_pointee, bool is_mutable) {
    const auto *pointee = gen_type(node, ast_pointee);
    return ir::PointerType::get(pointee, is_mutable);
}

const ir::Type *IrGen::gen_struct_type(const ast::Node *node, const std::vector<ast::StructField> &ast_fields) {
    // TODO: Size is already known here.
    std::vector<const ir::Type *> fields;
    for (const auto &ast_field : ast_fields) {
        // TODO: StructField needs to be its own ast node for the line number to be correct.
        fields.push_back(gen_type(node, ast_field.type));
    }
    const auto *type = ir::StructType::get(std::move(fields));
    m_struct_types.emplace(type, ast_fields);
    return type;
}

const ir::Type *IrGen::gen_type(const ast::Node *node, const ast::Type &ast_type) {
    switch (ast_type.kind()) {
    case ast::TypeKind::Invalid:
        return ir::InvalidType::get();
    case ast::TypeKind::Base:
        return gen_base_type(node, ast_type.base());
    case ast::TypeKind::Inferred:
        return ir::InferredType::get();
    case ast::TypeKind::Pointer:
        return gen_pointer_type(node, ast_type.pointee(), ast_type.is_mutable());
    case ast::TypeKind::Struct:
        return gen_struct_type(node, ast_type.struct_fields());
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
    inputs.reserve(asm_expr->inputs().size());
    for (const auto &[input, expr] : asm_expr->inputs()) {
        inputs.emplace_back(input, gen_expr(expr.get()));
    }
    return m_block->append<ir::InlineAsmInst>(asm_expr->instruction(), std::move(clobbers), std::move(inputs));
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
    std::vector<ir::Value *> args;
    for (const auto *ast_arg : call_expr->args()) {
        args.push_back(gen_expr(ast_arg));
    }
    auto it = std::find_if(m_program->begin(), m_program->end(), [call_expr](const ir::Function *function) {
        return function->name() == call_expr->name();
    });
    if (it == m_program->end()) {
        print_error(call_expr, "no function named '{}' in current context", call_expr->name());
        return ir::ConstantNull::get();
    }
    return m_block->append<ir::CallInst>(*it, std::move(args));
}

ir::Value *IrGen::gen_cast_expr(const ast::CastExpr *cast_expr) {
    // TODO: Always setting this to SignExtend is a bit misleading.
    auto *expr = gen_expr(cast_expr->val());
    const auto *type = gen_type(cast_expr, cast_expr->type());
    return m_block->append<ir::CastInst>(ir::CastOp::SignExtend, type, expr);
}

ir::Value *IrGen::gen_construct_expr(const ast::ConstructExpr *construct_expr) {
    std::vector<ir::Constant *> elems;
    for (const auto *arg : construct_expr->args()) {
        auto *arg_val = gen_expr(arg);
        elems.push_back(arg_val->as<ir::Constant>());
    }
    const auto *type = gen_base_type(construct_expr, construct_expr->name())->as<ir::StructType>();
    for (int i = 0; i < elems.size(); i++) {
        elems[i]->set_type(type->fields()[i]);
    }
    return ir::ConstantStruct::get(type, std::move(elems));
}

ir::Value *IrGen::gen_member_expr(const ast::MemberExpr *member_expr) {
    ir::Value *lhs = nullptr;
    {
        StateChanger deref_state_changer(m_deref_state, DerefState::DontDeref);
        StateChanger member_load_state_changer(m_member_load_state, MemberLoadState::DontLoad);
        lhs = gen_expr(member_expr->lhs());
    }
    const auto *rhs = member_expr->rhs()->as<ast::Symbol>();
    const auto *type = lhs->type();
    if (auto *var = lhs->as_or_null<ir::LocalVar>()) {
        type = var->var_type();
    }
    if (const auto *ptr_type = type->as_or_null<ir::PointerType>()) {
        type = ptr_type->pointee_type();
    }
    const auto *struct_type = type->as<ir::StructType>();
    const auto &ast_fields = m_struct_types.at(struct_type);
    auto it = std::find_if(ast_fields.begin(), ast_fields.end(), [rhs](const auto &field) {
        return field.name == rhs->name();
    });
    if (it == ast_fields.end()) {
        // TODO: Print struct type name.
        print_error(member_expr, "struct has no member named '{}'", rhs->name());
        return ir::ConstantNull::get();
    }
    int index = std::distance(ast_fields.begin(), it);
    auto *lea = get_member_ptr(lhs, index);
    lea->set_type(ir::PointerType::get(struct_type->fields()[index], true));
    if (m_member_load_state == MemberLoadState::Load) {
        return m_block->append<ir::LoadInst>(lea);
    }
    return lea;
}

ir::Value *IrGen::gen_num_lit(const ast::NumLit *num_lit) {
    // `+ 1` for signed bit.
    int bit_width = static_cast<int>(std::ceil(std::log2(num_lit->value()))) + 1;
    return ir::ConstantInt::get(ir::IntType::get_signed(bit_width), num_lit->value());
}

ir::Value *IrGen::gen_string_lit(const ast::StringLit *string_lit) {
    // TODO: Avoid copying string here?
    return ir::ConstantString::get(string_lit->value());
}

ir::Value *IrGen::gen_symbol(const ast::Symbol *symbol) {
    auto *var = m_scope_stack.peek().find_var(symbol->name());
    if (var == nullptr) {
        print_error(symbol, "no symbol named '{}' in current context", symbol->name());
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
    const auto *type = gen_type(decl_stmt, decl_stmt->type());
    auto *var = m_function->append_var(type, decl_stmt->is_mutable());
    var->set_name(decl_stmt->name());
    if (decl_stmt->init_val() != nullptr) {
        auto *init_val = gen_expr(decl_stmt->init_val());
        create_store(decl_stmt, var, init_val);
        if (type->is<ir::InferredType>()) {
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
    const auto *return_type = gen_type(function_decl, function_decl->return_type());
    m_function = m_program->append_function(function_decl->name(), return_type);
    for (const auto *ast_arg : function_decl->args()) {
        auto *arg = m_function->append_arg(ast_arg->is_mutable());
        arg->set_name(ast_arg->name());
        arg->set_type(gen_type(ast_arg, ast_arg->type()));
    }

    if (function_decl->externed()) {
        return;
    }

    m_block = m_function->append_block();
    m_scope_stack.emplace(m_scope_stack.peek());
    for (auto *arg : m_function->args()) {
        // TODO: This is a very lazy (clang-inspired) approach to arguments.
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
    const auto *type = gen_type(type_decl, type_decl->type());
    m_scope_stack.peek().put_type(type_decl->name(), type);
}

void IrGen::gen_decl(const ast::Node *decl) {
    switch (decl->kind()) {
    case ast::NodeKind::FunctionDecl:
        gen_function_decl(decl->as<ast::FunctionDecl>());
        break;
    case ast::NodeKind::TypeDecl:
        gen_type_decl(decl->as<ast::TypeDecl>());
        break;
    default:
        ENSURE_NOT_REACHED();
    }
}

} // namespace

std::unique_ptr<ir::Program> gen_ir(const ast::Root *root) {
    IrGen gen;
    for (const auto *decl : root->decls()) {
        gen.gen_decl(decl);
    }
    return gen.program();
}
