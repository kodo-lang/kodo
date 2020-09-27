#include <ir/Instructions.hh>

#include <ir/BasicBlock.hh>
#include <ir/Function.hh>
#include <ir/Visitor.hh>

#include <cassert>

// TODO: Check cast.
#define REPL_VALUE(val)                                                                                                \
    if (val == orig) {                                                                                                 \
        val->remove_user(this);                                                                                        \
        val = static_cast<decltype(val)>(repl);                                                                        \
        if (val != nullptr) {                                                                                          \
            val->add_user(this);                                                                                       \
        }                                                                                                              \
    }

BinaryInst::BinaryInst(BinaryOp op, Value *lhs, Value *rhs) : Instruction(KIND), m_op(op), m_lhs(lhs), m_rhs(rhs) {
    m_lhs->add_user(this);
    m_rhs->add_user(this);
    set_type(m_lhs->type());
}

BinaryInst::~BinaryInst() {
    if (m_lhs != nullptr) {
        m_lhs->remove_user(this);
    }
    if (m_rhs != nullptr) {
        m_rhs->remove_user(this);
    }
}

void BinaryInst::accept(Visitor *visitor) {
    visitor->visit(this);
}

// TODO: Figure out what's breaking clang-format here.
// clang-format off
void BinaryInst::replace_uses_of_with(Value *orig, Value *repl) {
    REPL_VALUE(m_lhs)
    REPL_VALUE(m_rhs)
} // clang-format on

BranchInst::BranchInst(BasicBlock *dst)
    : Instruction(KIND), m_dst(dst) {
    m_dst->add_user(this);
}

BranchInst::~BranchInst() {
    if (m_dst != nullptr) {
        m_dst->remove_user(this);
    }
}

void BranchInst::accept(Visitor *visitor) {
    visitor->visit(this);
}

void BranchInst::replace_uses_of_with(Value *orig, Value *repl) {
    assert(repl->is(ValueKind::BasicBlock));
    REPL_VALUE(m_dst)
}

CallInst::CallInst(Function *callee) : Instruction(KIND), m_callee(callee) {
    set_type(m_callee->return_type());
}

CallInst::~CallInst() {
    for (auto *arg : m_args) {
        if (arg != nullptr) {
            arg->remove_user(this);
        }
    }
}

void CallInst::add_arg(Value *arg) {
    m_args.push_back(arg);
}

void CallInst::accept(Visitor *visitor) {
    visitor->visit(this);
}

void CallInst::replace_uses_of_with(Value *orig, Value *repl) {
    for (auto *&arg : m_args) {
        REPL_VALUE(arg)
    }
}

LoadInst::LoadInst(Value *ptr) : Instruction(KIND), m_ptr(ptr) {
    m_ptr->add_user(this);
    assert(m_ptr->type()->is<PointerType>());
    set_type(m_ptr->type()->as<PointerType>()->pointee_type());
}

LoadInst::~LoadInst() {
    if (m_ptr != nullptr) {
        m_ptr->remove_user(this);
    }
}

void LoadInst::accept(Visitor *visitor) {
    visitor->visit(this);
}

// TODO: Figure out what's breaking clang-format here.
// clang-format off
void LoadInst::replace_uses_of_with(Value *orig, Value *repl) {
    REPL_VALUE(m_ptr)
} // clang-format on

StoreInst::StoreInst(Value *ptr, Value *val)
    : Instruction(KIND), m_ptr(ptr), m_val(val) {
    m_ptr->add_user(this);
    m_val->add_user(this);
}

StoreInst::~StoreInst() {
    if (m_ptr != nullptr) {
        m_ptr->remove_user(this);
    }
    if (m_val != nullptr) {
        m_val->remove_user(this);
    }
}

void StoreInst::accept(Visitor *visitor) {
    visitor->visit(this);
}

// TODO: Figure out what's breaking clang-format here.
// clang-format off
void StoreInst::replace_uses_of_with(Value *orig, Value *repl) {
    REPL_VALUE(m_ptr)
    REPL_VALUE(m_val)
} // clang-format on

RetInst::RetInst(Value *val)
    : Instruction(KIND), m_val(val) {
    m_val->add_user(this);
}

RetInst::~RetInst() {
    if (m_val != nullptr) {
        m_val->remove_user(this);
    }
}

void RetInst::accept(Visitor *visitor) {
    visitor->visit(this);
}

void RetInst::replace_uses_of_with(Value *orig, Value *repl) {
    REPL_VALUE(m_val)
}
