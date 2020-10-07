#include <ir/Dumper.hh>

#include <ir/BasicBlock.hh>
#include <ir/Constant.hh>
#include <ir/Function.hh>
#include <ir/Instructions.hh>
#include <ir/Program.hh>
#include <ir/Visitor.hh>
#include <support/Assert.hh>

#include <cstdint>
#include <iostream>
#include <string>
#include <unordered_map>

namespace ir {
namespace {

class FunctionDumper : public Visitor {
    std::unordered_map<const BasicBlock *, std::size_t> m_block_map;
    std::unordered_map<const Value *, std::size_t> m_value_map;

    std::string printable_block(const BasicBlock *block);
    std::string printable_value(const Value *value);

public:
    void dump(const Function *function);
    void visit(BinaryInst *) override;
    void visit(BranchInst *) override;
    void visit(CallInst *) override;
    void visit(CastInst *) override;
    void visit(CompareInst *) override;
    void visit(CondBranchInst *) override;
    void visit(LoadInst *) override;
    void visit(PhiInst *) override;
    void visit(StoreInst *) override;
    void visit(RetInst *) override;
};

std::string FunctionDumper::printable_block(const BasicBlock *block) {
    if (!m_block_map.contains(block)) {
        m_block_map.emplace(block, m_block_map.size());
    }
    return 'L' + std::to_string(m_block_map.at(block));
}

std::string FunctionDumper::printable_value(const Value *value) {
    if (value->has_name()) {
        return '%' + value->name();
    }
    if (const auto *constant = value->as_or_null<Constant>()) {
        return std::to_string(constant->value());
    }
    if (!m_value_map.contains(value)) {
        m_value_map.emplace(value, m_value_map.size());
    }
    return '%' + std::to_string(m_value_map.at(value));
}

void FunctionDumper::dump(const Function *function) {
    std::cout << function->return_type()->to_string() << ' ';
    std::cout << function->name() << '(';
    for (bool first = true; const auto *arg : function->args()) {
        if (!first) {
            std::cout << ", ";
        }
        first = false;
        std::cout << arg->type()->to_string() << ' ';
        std::cout << arg->name();
    }
    std::cout << ')';

    if (function->vars().empty() && (function->begin() == function->end())) {
        std::cout << ";\n";
        return;
    }

    std::cout << ":\n";
    std::cout << "  vars = [";
    for (bool first = true; const auto *var : function->vars()) {
        if (!first) {
            std::cout << ", ";
        }
        first = false;
        std::cout << var->var_type()->to_string() << ' ';
        std::cout << printable_value(var);
    }
    std::cout << "]\n";

    for (const auto *block : *function) {
        std::cout << "  " << printable_block(block) << ":\n";
        for (const auto *inst : *block) {
            // TODO: Add const visitor and remove const_cast here.
            std::cout << "    ";
            const_cast<Instruction *>(inst)->accept(this);
            std::cout << '\n';
        }
    }
}

void FunctionDumper::visit(BinaryInst *binary) {
    std::cout << printable_value(binary) << " = ";
    switch (binary->op()) {
    case BinaryOp::Add:
        std::cout << "add ";
        break;
    case BinaryOp::Sub:
        std::cout << "sub ";
        break;
    case BinaryOp::Mul:
        std::cout << "mul ";
        break;
    case BinaryOp::Div:
        std::cout << "div ";
        break;
    }
    std::cout << binary->lhs()->type()->to_string() << ' ' << printable_value(binary->lhs());
    std::cout << ", " << binary->rhs()->type()->to_string() << ' ' << printable_value(binary->rhs());
}

void FunctionDumper::visit(BranchInst *branch) {
    std::cout << "br " << printable_block(branch->dst());
}

void FunctionDumper::visit(CallInst *call) {
    if (!call->type()->is<VoidType>()) {
        std::cout << printable_value(call) << " = ";
    }
    std::cout << "call " << call->callee()->return_type()->to_string() << ' ';
    std::cout << '@' << call->callee()->name() << '(';
    for (bool first = true; auto *arg : call->args()) {
        if (!first) {
            std::cout << ", ";
        }
        first = false;
        std::cout << arg->type()->to_string() << ' ';
        std::cout << printable_value(arg);
    }
    std::cout << ')';
}

void FunctionDumper::visit(CastInst *cast) {
    auto cast_op_string = [](CastOp op) {
        switch (op) {
        case CastOp::IntToPtr:
            return "int_to_ptr";
        case CastOp::PtrToInt:
            return "ptr_to_int";
        case CastOp::SignExtend:
            return "sign_extend";
        case CastOp::Truncate:
            return "truncate";
        case CastOp::ZeroExtend:
            return "zero_extend";
        default:
            ENSURE_NOT_REACHED();
        }
    };
    std::cout << printable_value(cast) << " = ";
    std::cout << "cast " << cast->val()->type()->to_string() << ' ';
    std::cout << printable_value(cast->val()) << " -> ";
    std::cout << cast->type()->to_string() << " (";
    std::cout << cast_op_string(cast->op()) << ')';
}

void FunctionDumper::visit(CompareInst *compare) {
    std::cout << printable_value(compare) << " = ";
    switch (compare->op()) {
    case CompareOp::LessThan:
        std::cout << "cmp_lt ";
        break;
    case CompareOp::GreaterThan:
        std::cout << "cmp_gt ";
        break;
    }
    std::cout << compare->lhs()->type()->to_string() << ' ' << printable_value(compare->lhs());
    std::cout << ", " << compare->rhs()->type()->to_string() << ' ' << printable_value(compare->rhs());
}

void FunctionDumper::visit(CondBranchInst *cond_branch) {
    std::cout << "br " << cond_branch->cond()->type()->to_string();
    std::cout << ' ' << printable_value(cond_branch->cond());
    std::cout << ", " << printable_block(cond_branch->true_dst());
    std::cout << ", " << printable_block(cond_branch->false_dst());
}

void FunctionDumper::visit(LoadInst *load) {
    std::cout << printable_value(load) << " = ";
    std::cout << "load " << load->ptr()->type()->to_string() << ' ' << printable_value(load->ptr());
}

void FunctionDumper::visit(PhiInst *) {
    ASSERT_NOT_REACHED();
}

void FunctionDumper::visit(StoreInst *store) {
    std::cout << "store " << store->ptr()->type()->to_string() << ' ' << printable_value(store->ptr());
    std::cout << ", " << store->val()->type()->to_string() << ' ' << printable_value(store->val());
}

void FunctionDumper::visit(RetInst *ret) {
    std::cout << "ret " << ret->val()->type()->to_string() << ' ' << printable_value(ret->val());
}

} // namespace

void dump_ir(const Program *program) {
    for (const auto *function : *program) {
        FunctionDumper().dump(function);
    }
}

} // namespace ir
