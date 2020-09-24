#include <ir/Dumper.hh>

#include <ir/BasicBlock.hh>
#include <ir/Constant.hh>
#include <ir/Function.hh>
#include <ir/Instructions.hh>
#include <ir/Program.hh>
#include <ir/Visitor.hh>

#include <cassert>
#include <cstdint>
#include <iostream>
#include <string>
#include <unordered_map>

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
        return value->name();
    }
    if (const auto *constant = value->as<Constant>()) {
        return std::to_string(constant->value());
    }
    if (!m_value_map.contains(value)) {
        m_value_map.emplace(value, m_value_map.size());
    }
    return '%' + std::to_string(m_value_map.at(value));
}

void FunctionDumper::dump(const Function *function) {
    for (const auto *block : *function) {
        std::cout << printable_block(block) << ":\n";
        for (const auto *inst : *block) {
            // TODO: Add const visitor and remove const_cast here.
            std::cout << "  ";
            const_cast<Instruction *>(inst)->accept(this);
            std::cout << '\n';
        }
    }
}

void FunctionDumper::visit(BinaryInst *) {
    assert(false);
}

void FunctionDumper::visit(BranchInst *) {
    assert(false);
}

void FunctionDumper::visit(CompareInst *) {
    assert(false);
}

void FunctionDumper::visit(CondBranchInst *) {
    assert(false);
}

void FunctionDumper::visit(LoadInst *) {
    assert(false);
}

void FunctionDumper::visit(PhiInst *) {
    assert(false);
}

void FunctionDumper::visit(StoreInst *) {
    assert(false);
}

void FunctionDumper::visit(RetInst *ret) {
    std::cout << "ret "<< printable_value(ret->val());
}

} // namespace

void dump_ir(const Program *program) {
    for (const auto *function : *program) {
        FunctionDumper().dump(function);
    }
}
