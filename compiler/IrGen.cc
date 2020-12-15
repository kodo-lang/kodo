#include <IrGen.hh>

#include <ast/Nodes.hh>
#include <ir/BasicBlock.hh>
#include <ir/Constants.hh>
#include <ir/Function.hh>
#include <ir/Instructions.hh>
#include <ir/Program.hh>
#include <ir/Prototype.hh>
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
    std::unordered_map<std::string_view, ir::Value *> m_vars;

public:
    explicit Scope(const Scope *parent) : m_parent(parent) {}

    ir::Value *find_var(std::string_view name);
    void put_var(std::string_view name, ir::Value *value);
};

class IrGen {
    Box<ir::Program> m_program;
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

    ir::Value *create_call(const ast::CallExpr *, ir::Value *, ir::Value *);
    ir::Prototype *create_prototype(const ast::FunctionDecl *, const ir::Type * = nullptr);
    void create_store(const ast::Node *, ir::Value *, ir::Value *);
    ir::Prototype *find_prototype(const ir::Type *, const std::string &);
    ir::Value *get_member_ptr(ir::Value *, int);
    const ir::Type *get_type(const ast::Node *, const std::string &);
    const ir::Type *get_containing_type(const ast::Node *, const ast::Symbol *);

    const ir::Type *gen_base_type(const ast::Symbol *);
    const ir::Type *gen_pointer_type(const ast::PointerType *);
    const ir::Type *gen_struct_type(const ast::StructType *);
    const ir::Type *gen_trait_type(const ast::TraitType *);
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

    void gen_const_decl(const ast::ConstDecl *);
    void gen_function_decl(const ast::FunctionDecl *);
    void gen_type_decl(const ast::TypeDecl *);
    void gen_decl(const ast::Node *);

    Box<ir::Program> program() { return std::move(m_program); }
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
    m_program = Box<ir::Program>::create();
    m_scope_stack.emplace(/* parent */ nullptr);
}

std::string mangle(const ast::Symbol *name) {
    // TODO: This can be easily broken to make duplicate functions.
    std::string mangled_name;
    for (bool first = true; const auto &part : name->parts()) {
        if (!first) {
            mangled_name += "::";
        }
        first = false;
        mangled_name += part;
    }
    return std::move(mangled_name);
}

ir::Value *IrGen::create_call(const ast::CallExpr *call_expr, ir::Value *callee, ir::Value *this_arg) {
    std::vector<ir::Value *> args;
    if (this_arg != nullptr) {
        args.push_back(this_arg);
    }
    for (const auto *ast_arg : call_expr->args()) {
        args.push_back(gen_expr(ast_arg));
    }
    if (callee == nullptr) {
        print_error(call_expr, "no function named '{}' in current context", mangle(call_expr->name()));
        return ir::ConstantNull::get(m_program->invalid_type());
    }
    return m_block->append<ir::CallInst>(callee, std::move(args));
}

ir::Prototype *IrGen::create_prototype(const ast::FunctionDecl *function_decl, const ir::Type *containing_type) {
    if (containing_type == nullptr) {
        containing_type = get_containing_type(function_decl, function_decl->name());
    }

    // Create function type by first converting return type, then adding `*this` param if needed and finally, converting
    // the rest of the params.
    const auto *return_type = gen_type(function_decl->return_type());
    std::vector<const ir::Type *> params;
    if (function_decl->instance()) {
        ASSERT(containing_type != nullptr);
        params.push_back(m_program->pointer_type(containing_type, false));
    }
    for (const auto *ast_param : function_decl->args()) {
        params.push_back(gen_type(ast_param->type()));
    }

    const auto &name = function_decl->name()->parts().back();
    const auto *function_type = m_program->function_type(return_type, std::move(params));
    auto *prototype = new ir::Prototype(function_decl->externed(), name, function_type);
    if (containing_type == nullptr) {
        m_program->append_prototype(prototype);
    } else if (const auto *struct_type = ir::Type::base_as<ir::StructType>(containing_type)) {
        struct_type->add_prototype(prototype);
    }
    return prototype;
}

void IrGen::create_store(const ast::Node *node, ir::Value *ptr, ir::Value *val) {
    auto *store = m_block->append<ir::StoreInst>(ptr, val);
    store->set_line(node->line());
}

ir::Prototype *IrGen::find_prototype(const ir::Type *containing_type, const std::string &name) {
    const List<ir::Prototype> *prototype_list = nullptr;
    if (containing_type == nullptr) {
        prototype_list = &m_program->prototypes();
    } else if (const auto *struct_type = ir::Type::base_as<ir::StructType>(containing_type)) {
        prototype_list = &struct_type->prototypes();
    } else if (const auto *trait_type = ir::Type::base_as<ir::TraitType>(containing_type)) {
        prototype_list = &trait_type->prototypes();
    } else {
        ENSURE_NOT_REACHED();
    }
    for (auto *prototype : *prototype_list) {
        if (prototype->name() == name) {
            return prototype;
        }
    }
    return nullptr;
}

ir::Value *IrGen::get_member_ptr(ir::Value *ptr, int index) {
    std::vector<ir::Value *> indices;
    indices.reserve(2);
    indices.push_back(ir::ConstantInt::get(m_program->int_type(32, false), 0));
    indices.push_back(ir::ConstantInt::get(m_program->int_type(32, false), index));
    return m_block->append<ir::LeaInst>(ptr, std::move(indices));
}

const ir::Type *IrGen::get_type(const ast::Node *node, const std::string &name) {
    auto it =
        std::find_if(m_program->alias_types().begin(), m_program->alias_types().end(), [&name](const auto &alias) {
            return alias->name() == name;
        });
    if (it != m_program->alias_types().end()) {
        return **it;
    }
    print_error(node, "no type named '{}' in current context", name);
    return m_program->invalid_type();
}

const ir::Type *IrGen::get_containing_type(const ast::Node *node, const ast::Symbol *symbol) {
    const ir::Type *containing_type = nullptr;
    for (int i = 0; i < symbol->parts().size() - 1; i++) {
        containing_type = get_type(node, symbol->parts()[i]);
    }
    return containing_type;
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
    auto *type = m_program->make<ir::StructType>();
    for (const auto *struct_field : struct_type->fields()) {
        type->add_field(struct_field->name(), gen_type(struct_field->type()));
    }
    for (const auto *ast_trait : struct_type->implementing()) {
        const auto *symbol = ast_trait->as<ast::Symbol>();
        ASSERT(symbol->parts().size() == 1);
        type->add_implementing(get_type(struct_type, symbol->parts().back()));
    }
    return type;
}

const ir::Type *IrGen::gen_trait_type(const ast::TraitType *ast_type) {
    auto *type = m_program->make<ir::TraitType>();
    for (const auto *function_decl : ast_type->functions()) {
        type->add_prototype(create_prototype(function_decl, type));
    }
    return type;
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
    case ast::NodeKind::TraitType:
        return gen_trait_type(node->as<ast::TraitType>());
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
        inputs.emplace_back(input, gen_expr(*expr));
    }
    for (const auto &[output, expr] : asm_expr->outputs()) {
        StateChanger deref_state_changer(m_deref_state, DerefState::DontDeref);
        outputs.emplace_back(output, gen_expr(*expr));
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
    const auto *containing_type = get_containing_type(call_expr, call_expr->name());
    auto *callee = find_prototype(containing_type, call_expr->name()->parts().back());
    return create_call(call_expr, callee, nullptr);
}

ir::Value *IrGen::gen_cast_expr(const ast::CastExpr *cast_expr) {
    // TODO: Always setting this to SignExtend is a bit misleading.
    auto *expr = gen_expr(cast_expr->val());
    const auto *type = gen_type(cast_expr->type());
    return m_block->append<ir::CastInst>(ir::CastOp::SignExtend, type, expr);
}

ir::Value *IrGen::gen_construct_expr(const ast::ConstructExpr *construct_expr) {
    const auto *type = get_type(construct_expr, construct_expr->name());
    const auto *struct_type = ir::Type::base_as<ir::StructType>(type);
    ASSERT(construct_expr->args().size() == struct_type->fields().size());
    auto *tmp_var = m_function->append_var(type, true);
    for (int i = 0; const auto *arg : construct_expr->args()) {
        auto *lea = get_member_ptr(tmp_var, i);
        lea->set_type(m_program->pointer_type(struct_type->fields()[i++].type(), true));
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
    if (member_expr->is_pointer()) {
        lhs = m_block->append<ir::LoadInst>(lhs);
    }
    const auto [name, struct_type] = ir::Type::expand_alias<ir::StructType>(type);
    if (const auto *call_expr = member_expr->rhs()->as_or_null<ast::CallExpr>()) {
        // TODO: Further cleanup here.
        const auto &callee_name = call_expr->name()->parts().back();
        if (const auto *trait_type = ir::Type::base_as<ir::TraitType>(type)) {
            for (auto *prototype : trait_type->prototypes()) {
                if (prototype->name() == callee_name) {
                    return create_call(call_expr, prototype, lhs);
                }
            }
            print_error(call_expr, "trait '{}' has no function named '{}'", name, callee_name);
            return ir::ConstantNull::get(m_program->invalid_type());
        }
        auto *callee = find_prototype(ir::Type::base(type), call_expr->name()->parts().back());
        return create_call(call_expr, callee, lhs);
    }
    ASSERT(struct_type != nullptr);
    const auto *rhs = member_expr->rhs()->as<ast::Symbol>();
    ASSERT(rhs->parts().size() == 1);
    const auto &rhs_name = rhs->parts()[0];
    auto it = std::find_if(struct_type->fields().begin(), struct_type->fields().end(),
                           [&rhs_name](const ir::StructField &field) {
                               return field.name() == rhs_name;
                           });
    if (it == struct_type->fields().end()) {
        print_error(member_expr, "struct '{}' has no member named '{}'", name, rhs_name);
        return ir::ConstantNull::get(m_program->invalid_type());
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
    //    int bit_width = static_cast<int>(std::ceil(std::log2(std::max(1UL, num_lit->value())))) + 1;
    return ir::ConstantInt::get(m_program->invalid_type(), num_lit->value());
}

ir::Value *IrGen::gen_string_lit(const ast::StringLit *string_lit) {
    // TODO: Avoid copying string here?
    return ir::ConstantString::get(*m_program, string_lit->value());
}

ir::Value *IrGen::gen_symbol(const ast::Symbol *symbol) {
    ASSERT(symbol->parts().size() == 1);
    const auto &name = symbol->parts()[0];
    auto *var = m_scope_stack.peek().find_var(name);
    if (var == nullptr) {
        print_error(symbol, "no symbol named '{}' in current context", name);
        return ir::ConstantNull::get(m_program->invalid_type());
    }
    if (m_deref_state == DerefState::DontDeref || var->is<ir::Constant>()) {
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
        print_error(decl_stmt, "redeclaration of symbol '{}'", decl_stmt->name());
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

void IrGen::gen_const_decl(const ast::ConstDecl *const_decl) {
    if (m_scope_stack.peek().find_var(const_decl->name()) != nullptr) {
        print_error(const_decl, "redeclaration of symbol '{}'", const_decl->name());
        return;
    }
    auto *init_val = gen_expr(const_decl->init_val());
    if (!init_val->is<ir::Constant>()) {
        print_error(const_decl, "non-constant on right hand side of const declaration");
        return;
    }
    m_scope_stack.peek().put_var(const_decl->name(), init_val);
}

void IrGen::gen_function_decl(const ast::FunctionDecl *function_decl) {
    auto *prototype = create_prototype(function_decl);
    const auto *function_type = prototype->type()->as<ir::FunctionType>();
    m_function = m_program->append_function(prototype, mangle(function_decl->name()), function_type);
    if (prototype->externed()) {
        return;
    }

    // Create `*this` arg if needed.
    if (function_decl->instance()) {
        const auto *this_param = function_type->params()[0];
        auto *this_arg = m_function->append_arg(this_param->as<ir::PointerType>()->is_mutable());
        this_arg->set_name("this");
        this_arg->set_type(this_param);
    }

    // Create tangible arguments.
    for (int i = 0; i < function_decl->args().size(); i++) {
        const auto *ast_param = function_decl->args()[i];
        const auto *param = function_type->params()[function_decl->instance() ? i + 1 : i];
        auto *arg = m_function->append_arg(ast_param->is_mutable());
        arg->set_name(ast_param->name());
        arg->set_type(param);
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
    if (prototype->return_type()->is<ir::VoidType>() &&
        (return_block->empty() || return_block->terminator()->kind() != ir::InstKind::Ret)) {
        // TODO: Special return void instruction?
        return_block->append<ir::RetInst>(nullptr);
    }
}

void IrGen::gen_type_decl(const ast::TypeDecl *type_decl) {
    const auto *type = gen_type(type_decl->type());
    auto name = type_decl->name();
    m_program->alias_type(type, std::move(name));
}

void IrGen::gen_decl(const ast::Node *decl) {
    switch (decl->kind()) {
    case ast::NodeKind::ConstDecl:
        gen_const_decl(decl->as<ast::ConstDecl>());
        break;
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

Box<ir::Program> gen_ir(std::vector<Box<ast::Root>> &&roots) {
    IrGen gen;
    for (auto &root : roots) {
        for (const auto *decl : root->decls()) {
            gen.gen_decl(decl);
        }
    }
    return gen.program();
}
