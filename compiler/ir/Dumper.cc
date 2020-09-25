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
    std::string type_string(const Type *type);

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

std::string FunctionDumper::type_string(const Type *type) {
    switch (type->kind()) {
    case TypeKind::Invalid:
        assert(false);
    case TypeKind::Int:
        return "i" + std::to_string(type->as<IntType>()->bit_width());
    case TypeKind::Pointer:
        return type_string(type->as<PointerType>()->pointee_type()) + "*";
    }
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

void FunctionDumper::visit(LoadInst *load) {
    std::cout << printable_value(load) << " = ";
    std::cout << "load " << type_string(load->ptr()->type()) << ' ' << printable_value(load->ptr());
}

void FunctionDumper::visit(PhiInst *) {
    assert(false);
}

void FunctionDumper::visit(StoreInst *store) {
    std::cout << "store " << type_string(store->ptr()->type()) << ' ' << printable_value(store->ptr());
    std::cout << ", " << type_string(store->val()->type()) << ' ' << printable_value(store->val());
}

void FunctionDumper::visit(RetInst *ret) {
    std::cout << "ret " << type_string(ret->val()->type()) << ' ' << printable_value(ret->val());
}

} // namespace

void dump_ir(const Program *program) {
    for (const auto *function : *program) {
        FunctionDumper().dump(function);
    }
}
