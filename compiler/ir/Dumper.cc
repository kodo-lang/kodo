#include <ir/Dumper.hh>

#include <ir/BasicBlock.hh>
#include <ir/Constants.hh>
#include <ir/Function.hh>
#include <ir/Instructions.hh>
#include <ir/Program.hh>
#include <ir/Visitor.hh>
#include <support/Assert.hh>

#include <fmt/core.h>

#include <cstdint>
#include <string>
#include <unordered_map>

namespace ir {
namespace {

class FunctionDumper : public Visitor {
    std::unordered_map<const BasicBlock *, std::size_t> m_block_map;
    std::unordered_map<const Value *, std::size_t> m_stack_map;
    std::unordered_map<const Value *, std::size_t> m_value_map;

    std::string printable_block(const BasicBlock *block);
    std::string printable_value(const Value *value, bool no_type = false);

public:
    void dump(const Function *function);
    void visit(BinaryInst *) override;
    void visit(BranchInst *) override;
    void visit(CallInst *) override;
    void visit(CastInst *) override;
    void visit(CompareInst *) override;
    void visit(CondBranchInst *) override;
    void visit(CopyInst *) override;
    void visit(InlineAsmInst *) override;
    void visit(LeaInst *) override;
    void visit(LoadInst *) override;
    void visit(PhiInst *) override;
    void visit(StoreInst *) override;
    void visit(RetInst *) override;
};

std::string printable_constant(const Constant *constant) {
    switch (constant->kind()) {
    case ConstantKind::Int:
        return std::to_string(constant->as<ConstantInt>()->value());
    case ConstantKind::Null:
        return "null";
    case ConstantKind::String:
        return constant->as<ConstantString>()->value();
    default:
        ENSURE_NOT_REACHED();
    }
}

std::string FunctionDumper::printable_block(const BasicBlock *block) {
    if (!m_block_map.contains(block)) {
        m_block_map.emplace(block, m_block_map.size());
    }
    return 'L' + std::to_string(m_block_map.at(block));
}

std::string FunctionDumper::printable_value(const Value *value, bool no_type) {
    // TODO: Proper undefined value.
    if (value == nullptr) {
        return "undef";
    }
    if (const auto *function = value->as_or_null<Function>()) {
        return '@' + function->name();
    }
    std::string ret;
    if (!no_type) {
        ret += value->type()->to_string() + ' ';
    }
    if (const auto *constant = value->as_or_null<Constant>()) {
        ret += printable_constant(constant);
        return std::move(ret);
    }
    ret += '%';
    // TODO: Remove LocalVar check when debug info is split (since local vars won't have names anymore).
    if (value->has_name() && !value->is<LocalVar>()) {
        ret += value->name();
        return std::move(ret);
    }
    auto &map = value->is<LocalVar>() ? m_stack_map : m_value_map;
    ret += value->is<LocalVar>() ? 's' : 'v';
    if (!map.contains(value)) {
        map.emplace(value, map.size());
    }
    ret += std::to_string(map.at(value));
    return std::move(ret);
}

void FunctionDumper::dump(const Function *function) {
    fmt::print("fn {}(", printable_value(function));
    for (bool first = true; const auto *arg : function->args()) {
        if (!first) {
            fmt::print(", ");
        }
        first = false;
        fmt::print("{} {}: {}", arg->is_mutable() ? "var" : "let", printable_value(arg, true),
                   arg->type()->to_string());
    }
    fmt::print(")");

    if (!function->return_type()->is<VoidType>()) {
        fmt::print(": {}", function->return_type()->to_string());
    }

    if (function->externed()) {
        fmt::print(";\n");
        return;
    }

    fmt::print(" {{\n");
    for (const auto *var : function->vars()) {
        fmt::print("  {} {}: {}\n", var->is_mutable() ? "var" : "let", printable_value(var, true),
                   var->var_type()->to_string());
    }

    for (const auto *block : *function) {
        fmt::print("  {} {{\n", printable_block(block));
        for (const auto *inst : *block) {
            // TODO: Add const visitor and remove const_cast here.
            fmt::print("    ");
            if (inst->type() != nullptr && !inst->type()->is<InvalidType>()) {
                // TODO: Bit hacky.
                if (!inst->as_or_null<CallInst>() || !inst->type()->is<VoidType>()) {
                    fmt::print("{} = ", printable_value(inst, true));
                }
            }
            const_cast<Instruction *>(inst)->accept(this);
            fmt::print("\n");
        }
        fmt::print("  }}\n");
    }
    fmt::print("}}\n");
}

void FunctionDumper::visit(BinaryInst *binary) {
    //    fmt::print("{} = ");
    switch (binary->op()) {
    case BinaryOp::Add:
        fmt::print("add ");
        break;
    case BinaryOp::Sub:
        fmt::print("sub ");
        break;
    case BinaryOp::Mul:
        fmt::print("mul ");
        break;
    case BinaryOp::Div:
        fmt::print("div ");
        break;
    }
    fmt::print("{}, {}", printable_value(binary->lhs()), printable_value(binary->rhs()));
}

void FunctionDumper::visit(BranchInst *branch) {
    fmt::print("br {}", printable_block(branch->dst()));
}

void FunctionDumper::visit(CallInst *call) {
    fmt::print("call {} {}(", call->type()->to_string(), printable_value(call->callee()));
    for (bool first = true; auto *arg : call->args()) {
        if (!first) {
            fmt::print(", ");
        }
        first = false;
        fmt::print("{}", printable_value(arg));
    }
    fmt::print(")");
}

void FunctionDumper::visit(CastInst *cast) {
    auto cast_op_string = [](CastOp op) {
        switch (op) {
        case CastOp::IntToPtr:
            return "int_to_ptr";
        case CastOp::PtrToInt:
            return "ptr_to_int";
        case CastOp::SignExtend:
            return "sext";
        case CastOp::Truncate:
            return "trunc";
        case CastOp::ZeroExtend:
            return "zext";
        default:
            ENSURE_NOT_REACHED();
        }
    };
    fmt::print("cast {} -> {} ({})", printable_value(cast->val()), cast->type()->to_string(),
               cast_op_string(cast->op()));
}

void FunctionDumper::visit(CompareInst *compare) {
    fmt::print("cmp_");
    switch (compare->op()) {
    case CompareOp::LessThan:
        fmt::print("lt ");
        break;
    case CompareOp::GreaterThan:
        fmt::print("gt ");
        break;
    }
    fmt::print("{}, {}", printable_value(compare->lhs()), printable_value(compare->rhs()));
}

void FunctionDumper::visit(CondBranchInst *cond_branch) {
    fmt::print("br {}, {}, {}", printable_value(cond_branch->cond()), printable_block(cond_branch->true_dst()),
               printable_block(cond_branch->false_dst()));
}

void FunctionDumper::visit(CopyInst *copy) {
    fmt::print("copy {} -> {} * {}", printable_value(copy->src()), printable_value(copy->dst()),
               printable_value(copy->len()));
}

void FunctionDumper::visit(InlineAsmInst *inline_asm) {
    fmt::print("asm {} \"{}\"", inline_asm->type()->to_string(), inline_asm->instruction());
    for (const auto &clobber : inline_asm->clobbers()) {
        fmt::print(", clobber({})", clobber);
    }
    for (const auto &[input, value] : inline_asm->inputs()) {
        fmt::print(", input({}, {})", input, printable_value(value));
    }
    for (const auto &[output, value] : inline_asm->outputs()) {
        fmt::print(", output({}, {})", output, printable_value(value));
    }
}

void FunctionDumper::visit(LeaInst *lea) {
    fmt::print("lea {}, {}", lea->type()->to_string(), printable_value(lea->ptr()));
    for (auto *index : lea->indices()) {
        fmt::print(", {}", printable_value(index));
    }
}

void FunctionDumper::visit(LoadInst *load) {
    fmt::print("load {}", printable_value(load->ptr()));
}

void FunctionDumper::visit(PhiInst *phi) {
    fmt::print("phi (");
    for (bool first = true; auto [block, value] : phi->incoming()) {
        if (!first) {
            fmt::print(", ");
        }
        first = false;
        fmt::print("{}: {}", printable_block(block), printable_value(value));
    }
    fmt::print(")");
}

void FunctionDumper::visit(StoreInst *store) {
    fmt::print("store {}, {}", printable_value(store->ptr()), printable_value(store->val()));
}

void FunctionDumper::visit(RetInst *ret) {
    if (ret->val() == nullptr) {
        fmt::print("ret void");
        return;
    }
    fmt::print("ret {}", printable_value(ret->val()));
}

} // namespace

void Dumper::run(Function *function) {
    FunctionDumper().dump(function);
}

} // namespace ir
