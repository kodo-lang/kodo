#pragma once

#include <ir/Instruction.hh>
#include <ir/Value.hh>

class BasicBlock;

enum class BinaryOp {
    Add,
    Sub,
    Mul,
    Div,
};

class BinaryInst : public Instruction {
    const BinaryOp m_op;
    Value *m_lhs;
    Value *m_rhs;

public:
    BinaryInst(BinaryOp op, Value *lhs, Value *rhs);
    BinaryInst(const BinaryInst &) = delete;
    BinaryInst(BinaryInst &&) = delete;
    ~BinaryInst() override;

    BinaryInst &operator=(const BinaryInst &) = delete;
    BinaryInst &operator=(BinaryInst &&) = delete;

    void accept(Visitor *visitor) override;
    void replace_uses_of_with(Value *orig, Value *repl) override;

    BinaryOp op() const { return m_op; }
    Value *lhs() const { return m_lhs; }
    Value *rhs() const { return m_rhs; }
};

class BranchInst : public Instruction {
    BasicBlock *m_dst;

public:
    explicit BranchInst(BasicBlock *dst);
    BranchInst(const BranchInst &) = delete;
    BranchInst(BranchInst &&) = delete;
    ~BranchInst() override;

    BranchInst &operator=(const BranchInst &) = delete;
    BranchInst &operator=(BranchInst &&) = delete;

    void accept(Visitor *visitor) override;
    void replace_uses_of_with(Value *orig, Value *repl) override;

    BasicBlock *dst() const { return m_dst; }
};

class LoadInst : public Instruction {
    Value *m_ptr;

public:
    explicit LoadInst(Value *ptr);
    LoadInst(const LoadInst &) = delete;
    LoadInst(LoadInst &&) = delete;
    ~LoadInst() override;

    LoadInst &operator=(const LoadInst &) = delete;
    LoadInst &operator=(LoadInst &&) = delete;

    void accept(Visitor *visitor) override;
    void replace_uses_of_with(Value *orig, Value *repl) override;

    Value *ptr() const { return m_ptr; }
};

class StoreInst : public Instruction {
    Value *m_ptr;
    Value *m_val;

public:
    StoreInst(Value *ptr, Value *val);
    StoreInst(const StoreInst &) = delete;
    StoreInst(StoreInst &&) = delete;
    ~StoreInst() override;

    StoreInst &operator=(const StoreInst &) = delete;
    StoreInst &operator=(StoreInst &&) = delete;

    void accept(Visitor *visitor) override;
    void replace_uses_of_with(Value *orig, Value *repl) override;

    Value *ptr() const { return m_ptr; }
    Value *val() const { return m_val; }
};

class RetInst : public Instruction {
    Value *m_val;

public:
    explicit RetInst(Value *val);
    RetInst(const RetInst &) = delete;
    RetInst(RetInst &&) = delete;
    ~RetInst() override;

    RetInst &operator=(const RetInst &) = delete;
    RetInst &operator=(RetInst &&) = delete;

    void accept(Visitor *visitor) override;
    void replace_uses_of_with(Value *orig, Value *repl) override;

    Value *val() const { return m_val; }
};
