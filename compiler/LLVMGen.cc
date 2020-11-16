#include <LLVMGen.hh>

#include <ir/BasicBlock.hh>
#include <ir/Constants.hh>
#include <ir/Function.hh>
#include <ir/Instructions.hh>
#include <ir/Program.hh>
#include <support/Assert.hh>

#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/InlineAsm.h>

#include <unordered_map>
#include <utility>
#include <vector>

namespace {

class LLVMGen {
    llvm::LLVMContext *const m_llvm_context;

    std::unique_ptr<llvm::Module> m_llvm_module;
    llvm::Function *m_llvm_function{nullptr};
    llvm::BasicBlock *m_llvm_block{nullptr};
    llvm::IRBuilder<> m_llvm_builder;

    std::unordered_map<const ir::Argument *, llvm::Argument *> m_arg_map;
    std::unordered_map<const ir::BasicBlock *, llvm::BasicBlock *> m_block_map;
    std::unordered_map<const ir::Value *, llvm::Value *> m_value_map;

public:
    explicit LLVMGen(llvm::LLVMContext *llvm_context);

    llvm::StructType *llvm_struct_type(const ir::StructType *);
    llvm::Type *llvm_type(const ir::Type *);
    llvm::Value *llvm_value(const ir::Value *);

    llvm::Value *gen_constant_int(const ir::ConstantInt *);
    llvm::Value *gen_constant_string(const ir::ConstantString *);
    llvm::Value *gen_constant_struct(const ir::ConstantStruct *);

    llvm::Value *gen_binary(const ir::BinaryInst *);
    llvm::Value *gen_call(const ir::CallInst *);
    llvm::Value *gen_cast(const ir::CastInst *);
    llvm::Value *gen_compare(const ir::CompareInst *);
    llvm::Value *gen_inline_asm(const ir::InlineAsmInst *);
    llvm::Value *gen_lea(const ir::LeaInst *);
    llvm::Value *gen_load(const ir::LoadInst *);
    llvm::Value *gen_phi(const ir::PhiInst *);
    void gen_branch(const ir::BranchInst *);
    void gen_cond_branch(const ir::CondBranchInst *);
    void gen_copy(const ir::CopyInst *);
    void gen_store(const ir::StoreInst *);
    void gen_ret(const ir::RetInst *);

    llvm::Value *gen_argument(const ir::Argument *);
    llvm::Value *gen_constant(const ir::Constant *);
    llvm::Value *gen_instruction(const ir::Instruction *);
    llvm::Value *gen_value(const ir::Value *);

    void gen_block(const ir::BasicBlock *);
    void gen_function(const ir::Function *);
    void gen_program(const ir::Program *);

    std::unique_ptr<llvm::Module> module() { return std::move(m_llvm_module); }
};

LLVMGen::LLVMGen(llvm::LLVMContext *llvm_context) : m_llvm_context(llvm_context), m_llvm_builder(*llvm_context) {
    m_llvm_module = std::make_unique<llvm::Module>("main", *m_llvm_context);
}

llvm::StructType *LLVMGen::llvm_struct_type(const ir::StructType *struct_type) {
    // TODO: Size is already known here.
    std::vector<llvm::Type *> fields;
    for (const auto *field : struct_type->fields()) {
        fields.push_back(llvm_type(field));
    }
    return llvm::StructType::get(*m_llvm_context, fields);
}

llvm::Type *LLVMGen::llvm_type(const ir::Type *type) {
    switch (type->kind()) {
    case ir::TypeKind::Bool:
        return llvm::Type::getInt1Ty(*m_llvm_context);
    case ir::TypeKind::Int:
        return llvm::Type::getIntNTy(*m_llvm_context, type->as<ir::IntType>()->bit_width());
    case ir::TypeKind::Pointer:
        return llvm::PointerType::get(llvm_type(type->as<ir::PointerType>()->pointee_type()), 0);
    case ir::TypeKind::Struct:
        return llvm_struct_type(type->as<ir::StructType>());
    case ir::TypeKind::Void:
        return llvm::Type::getVoidTy(*m_llvm_context);
    default:
        ENSURE_NOT_REACHED();
    }
}

llvm::Value *LLVMGen::llvm_value(const ir::Value *value) {
    if (!m_value_map.contains(value)) {
        m_value_map.emplace(value, gen_value(value));
    }
    auto *llvm_value = m_value_map.at(value);
    if (value->has_name()) {
        llvm_value->setName(value->name());
    }
    return llvm_value;
}

llvm::Value *LLVMGen::gen_constant_int(const ir::ConstantInt *constant_int) {
    return llvm::ConstantInt::get(llvm_type(constant_int->type()), constant_int->value());
}

llvm::Value *LLVMGen::gen_constant_string(const ir::ConstantString *constant_string) {
    return m_llvm_builder.CreateGlobalStringPtr(constant_string->value());
}

llvm::Value *LLVMGen::gen_constant_struct(const ir::ConstantStruct *constant_struct) {
    // TODO: Size is already known here.
    std::vector<llvm::Constant *> elems;
    for (const auto *elem : constant_struct->elems()) {
        elems.push_back(llvm::dyn_cast<llvm::Constant>(llvm_value(elem)));
    }
    auto *type = llvm_struct_type(constant_struct->struct_type());
    auto *value = llvm::ConstantStruct::get(type, elems);
    auto *global = new llvm::GlobalVariable(*m_llvm_module, type, true, llvm::GlobalValue::PrivateLinkage, value);
    global->setUnnamedAddr(llvm::GlobalValue::UnnamedAddr::Global);
    return global;
}

llvm::Value *LLVMGen::gen_binary(const ir::BinaryInst *binary) {
    auto *lhs = llvm_value(binary->lhs());
    auto *rhs = llvm_value(binary->rhs());
    switch (binary->op()) {
    case ir::BinaryOp::Add:
        return m_llvm_builder.CreateAdd(lhs, rhs);
    case ir::BinaryOp::Sub:
        return m_llvm_builder.CreateSub(lhs, rhs);
    case ir::BinaryOp::Mul:
        return m_llvm_builder.CreateMul(lhs, rhs);
    case ir::BinaryOp::Div:
        return m_llvm_builder.CreateSDiv(lhs, rhs);
    default:
        ENSURE_NOT_REACHED();
    }
}

llvm::Value *LLVMGen::gen_call(const ir::CallInst *call) {
    std::vector<llvm::Value *> args;
    for (auto *arg : call->args()) {
        args.push_back(llvm_value(arg));
    }
    auto *callee = m_llvm_module->getFunction(call->callee()->name());
    return m_llvm_builder.CreateCall(callee, args);
}

llvm::Value *LLVMGen::gen_cast(const ir::CastInst *cast) {
    auto *type = llvm_type(cast->type());
    auto *value = llvm_value(cast->val());
    switch (cast->op()) {
    case ir::CastOp::SignExtend:
        return m_llvm_builder.CreateSExt(value, type);
    case ir::CastOp::ZeroExtend:
        return m_llvm_builder.CreateZExt(value, type);
    default:
        ENSURE_NOT_REACHED();
    }
}

llvm::Value *LLVMGen::gen_compare(const ir::CompareInst *compare) {
    auto *lhs = llvm_value(compare->lhs());
    auto *rhs = llvm_value(compare->rhs());
    switch (compare->op()) {
    case ir::CompareOp::LessThan:
        return m_llvm_builder.CreateICmpSLT(lhs, rhs);
    case ir::CompareOp::GreaterThan:
        return m_llvm_builder.CreateICmpSGT(lhs, rhs);
    default:
        ENSURE_NOT_REACHED();
    }
}

llvm::Value *LLVMGen::gen_inline_asm(const ir::InlineAsmInst *inline_asm) {
    std::string llvm_str;
    bool first = true;

    // Build inputs.
    std::vector<llvm::Value *> args;
    std::vector<llvm::Type *> arg_types;
    args.reserve(inline_asm->inputs().size());
    arg_types.reserve(inline_asm->inputs().size());
    for (const auto &[input, value] : inline_asm->inputs()) {
        if (!first) {
            llvm_str += ',';
        }
        first = false;
        llvm_str += '{' + input + '}';
        args.push_back(llvm_value(value));
        arg_types.push_back(llvm_type(value->type()));
    }

    // Build outputs.
    std::vector<llvm::Type *> ret_types;
    ret_types.reserve(inline_asm->outputs().size());
    for (const auto &[output, value] : inline_asm->outputs()) {
        if (!first) {
            llvm_str += ',';
        }
        first = false;
        llvm_str += "={" + output + '}';
        ret_types.push_back(llvm_type(value->as<ir::LocalVar>()->var_type()));
    }

    // Build clobbers.
    for (const auto &clobber : inline_asm->clobbers()) {
        if (!first) {
            llvm_str += ',';
        }
        first = false;
        llvm_str += "~{" + clobber + '}';
    }

    auto *ret_type = llvm::Type::getVoidTy(*m_llvm_context);
    if (!ret_types.empty()) {
        ret_type = llvm::StructType::get(*m_llvm_context, ret_types, false);
    }
    auto *type = llvm::FunctionType::get(ret_type, arg_types, false);
    auto *llvm_asm =
        llvm::InlineAsm::get(type, inline_asm->instruction(), llvm_str, true, true, llvm::InlineAsm::AD_Intel);
    auto *call = m_llvm_builder.CreateCall(llvm_asm, args);
    for (int i = 0; const auto &[output, value] : inline_asm->outputs()) {
        std::vector<unsigned int> indices;
        indices.push_back(i);
        auto *extract = m_llvm_builder.CreateExtractValue(call, indices);
        m_llvm_builder.CreateStore(extract, llvm_value(value));
    }
    return call;
}

llvm::Value *LLVMGen::gen_lea(const ir::LeaInst *lea) {
    auto *ptr = llvm_value(lea->ptr());
    // TODO: Size is already known here.
    std::vector<llvm::Value *> indices;
    for (auto *index : lea->indices()) {
        indices.push_back(llvm_value(index));
    }
    return m_llvm_builder.CreateInBoundsGEP(ptr, indices);
}

llvm::Value *LLVMGen::gen_load(const ir::LoadInst *load) {
    return m_llvm_builder.CreateLoad(llvm_value(load->ptr()));
}

llvm::Value *LLVMGen::gen_phi(const ir::PhiInst *phi) {
    auto *llvm_phi = m_llvm_builder.CreatePHI(llvm_type(phi->type()), phi->incoming().size());
    for (auto [block, value] : phi->incoming()) {
        llvm_phi->addIncoming(llvm_value(value), m_block_map.at(block));
    }
    return llvm_phi;
}

void LLVMGen::gen_branch(const ir::BranchInst *branch) {
    m_llvm_builder.CreateBr(m_block_map.at(branch->dst()));
}

void LLVMGen::gen_cond_branch(const ir::CondBranchInst *cond_branch) {
    auto *cond = llvm_value(cond_branch->cond());
    auto *true_dst = m_block_map.at(cond_branch->true_dst());
    auto *false_dst = m_block_map.at(cond_branch->false_dst());
    m_llvm_builder.CreateCondBr(cond, true_dst, false_dst);
}

void LLVMGen::gen_copy(const ir::CopyInst *copy) {
    auto *dst = llvm_value(copy->dst());
    auto *src = llvm_value(copy->src());
    auto *len = llvm_value(copy->len());
    m_llvm_builder.CreateMemCpy(dst, llvm::MaybeAlign(), src, llvm::MaybeAlign(), len);
}

void LLVMGen::gen_store(const ir::StoreInst *store) {
    m_llvm_builder.CreateStore(llvm_value(store->val()), llvm_value(store->ptr()));
}

void LLVMGen::gen_ret(const ir::RetInst *ret) {
    if (ret->val() == nullptr) {
        m_llvm_builder.CreateRetVoid();
        return;
    }
    m_llvm_builder.CreateRet(llvm_value(ret->val()));
}

llvm::Value *LLVMGen::gen_argument(const ir::Argument *argument) {
    return m_arg_map.at(argument);
}

llvm::Value *LLVMGen::gen_constant(const ir::Constant *constant) {
    switch (constant->kind()) {
    case ir::ConstantKind::Int:
        return gen_constant_int(constant->as<ir::ConstantInt>());
    case ir::ConstantKind::String:
        return gen_constant_string(constant->as<ir::ConstantString>());
    case ir::ConstantKind::Struct:
        return gen_constant_struct(constant->as<ir::ConstantStruct>());
    default:
        ENSURE_NOT_REACHED();
    }
}

llvm::Value *LLVMGen::gen_instruction(const ir::Instruction *instruction) {
    switch (instruction->kind()) {
    case ir::InstKind::Binary:
        return gen_binary(instruction->as<ir::BinaryInst>());
    case ir::InstKind::Branch:
        gen_branch(instruction->as<ir::BranchInst>());
        return nullptr;
    case ir::InstKind::Call:
        return gen_call(instruction->as<ir::CallInst>());
    case ir::InstKind::Cast:
        return gen_cast(instruction->as<ir::CastInst>());
    case ir::InstKind::Compare:
        return gen_compare(instruction->as<ir::CompareInst>());
    case ir::InstKind::CondBranch:
        gen_cond_branch(instruction->as<ir::CondBranchInst>());
        return nullptr;
    case ir::InstKind::Copy:
        gen_copy(instruction->as<ir::CopyInst>());
        return nullptr;
    case ir::InstKind::InlineAsm:
        return gen_inline_asm(instruction->as<ir::InlineAsmInst>());
    case ir::InstKind::Lea:
        return gen_lea(instruction->as<ir::LeaInst>());
    case ir::InstKind::Load:
        return gen_load(instruction->as<ir::LoadInst>());
    case ir::InstKind::Phi:
        return gen_phi(instruction->as<ir::PhiInst>());
    case ir::InstKind::Store:
        gen_store(instruction->as<ir::StoreInst>());
        return nullptr;
    case ir::InstKind::Ret:
        gen_ret(instruction->as<ir::RetInst>());
        return nullptr;
    default:
        ENSURE_NOT_REACHED();
    }
}

llvm::Value *LLVMGen::gen_value(const ir::Value *value) {
    switch (value->kind()) {
    case ir::ValueKind::Argument:
        return gen_argument(value->as<ir::Argument>());
    case ir::ValueKind::Constant:
        return gen_constant(value->as<ir::Constant>());
    case ir::ValueKind::Instruction:
        return gen_instruction(value->as<ir::Instruction>());
    default:
        ENSURE_NOT_REACHED();
    }
}

void LLVMGen::gen_block(const ir::BasicBlock *block) {
    auto *new_block = m_block_map.at(block);
    if (m_llvm_block->empty() || !m_llvm_block->back().isTerminator()) {
        m_llvm_builder.CreateBr(new_block);
    }
    m_llvm_builder.SetInsertPoint(m_llvm_block = new_block);
    for (const auto *inst : *block) {
        llvm_value(inst);
    }
}

void LLVMGen::gen_function(const ir::Function *function) {
    // If the function has no blocks, return early.
    if (function->begin() == function->end()) {
        ASSERT(function->vars().empty());
        return;
    }

    m_llvm_function = m_llvm_module->getFunction(function->name());
    ASSERT(m_llvm_function != nullptr);

    m_llvm_block = llvm::BasicBlock::Create(*m_llvm_context, "vars", m_llvm_function);
    m_llvm_builder.SetInsertPoint(m_llvm_block);
    for (const auto *block : *function) {
        auto *llvm_block = llvm::BasicBlock::Create(*m_llvm_context, "", m_llvm_function);
        m_block_map.emplace(block, llvm_block);
    }

    for (int i = 0; const auto *arg : function->args()) {
        m_arg_map.emplace(arg, m_llvm_function->getArg(i++));
    }
    for (const auto *var : function->vars()) {
        auto *alloca = m_llvm_builder.CreateAlloca(llvm_type(var->var_type()));
        m_value_map.emplace(var, alloca);
    }
    for (const auto *block : *function) {
        gen_block(block);
    }
}

void LLVMGen::gen_program(const ir::Program *program) {
    for (auto *function : *program) {
        std::vector<llvm::Type *> arg_types;
        for (const auto *arg : function->args()) {
            ASSERT(arg->has_type());
            arg_types.push_back(llvm_type(arg->type()));
        }

        ASSERT(m_llvm_module->getFunction(function->name()) == nullptr);
        auto *function_type = llvm::FunctionType::get(llvm_type(function->return_type()), arg_types, false);
        llvm::Function::Create(function_type, llvm::Function::ExternalLinkage, function->name(), *m_llvm_module);
    }
    for (auto *function : *program) {
        gen_function(function);
    }
}

} // namespace

std::unique_ptr<llvm::Module> gen_llvm(const ir::Program *program, llvm::LLVMContext *llvm_context) {
    LLVMGen gen(llvm_context);
    gen.gen_program(program);
    return gen.module();
}
