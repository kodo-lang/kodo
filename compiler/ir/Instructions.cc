#include <ir/Instructions.hh>

#include <ir/BasicBlock.hh>
#include <ir/Function.hh>
#include <ir/Types.hh>
#include <ir/Visitor.hh>
#include <support/Assert.hh>

namespace ir {

// TODO: Check cast.
#define REPL_VALUE(val)                                                                                                \
    if (val == orig) {                                                                                                 \
        val->remove_user(this);                                                                                        \
        val = static_cast<decltype(val)>(repl);                                                                        \
        if (val != nullptr) {                                                                                          \
            val->add_user(this);                                                                                       \
        }                                                                                                              \
    }

BinaryInst::BinaryInst(BasicBlock *parent, BinaryOp op, Value *lhs, Value *rhs)
    : Instruction(KIND, parent), m_op(op), m_lhs(lhs), m_rhs(rhs) {
    m_lhs->add_user(this);
    m_rhs->add_user(this);
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

BranchInst::BranchInst(BasicBlock *parent, BasicBlock *dst)
    : Instruction(KIND, parent), m_dst(dst) {
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
    ASSERT(repl->kind() == ValueKind::BasicBlock);
    REPL_VALUE(m_dst)
}

CallInst::CallInst(BasicBlock *parent, Value *callee, std::vector<Value *> args)
    : Instruction(KIND, parent), m_callee(callee), m_args(std::move(args)) {
    for (auto *arg : m_args) {
        arg->add_user(this);
    }
    m_callee->add_user(this);
    set_type(callee_function_type()->return_type());
}

CallInst::~CallInst() {
    for (auto *arg : m_args) {
        if (arg != nullptr) {
            arg->remove_user(this);
        }
    }
    if (m_callee != nullptr) {
        m_callee->remove_user(this);
    }
}

void CallInst::accept(Visitor *visitor) {
    visitor->visit(this);
}

void CallInst::replace_uses_of_with(Value *orig, Value *repl) {
    for (auto *&arg : m_args) {
        REPL_VALUE(arg)
    }
    REPL_VALUE(m_callee)
}

const FunctionType *CallInst::callee_function_type() const {
    if (const auto *pointer_type = m_callee->type()->as_or_null<PointerType>()) {
        return pointer_type->pointee_type()->as<FunctionType>();
    }
    return m_callee->type()->as<FunctionType>();
}

CastInst::CastInst(BasicBlock *parent, CastOp op, const Type *type, Value *val)
    : Instruction(KIND, parent), m_op(op), m_val(val) {
    m_val->add_user(this);
    set_type(type);
}

CastInst::~CastInst() {
    if (m_val != nullptr) {
        m_val->remove_user(this);
    }
}

void CastInst::accept(Visitor *visitor) {
    visitor->visit(this);
}

// TODO: Figure out what's breaking clang-format here.
// clang-format off
void CastInst::replace_uses_of_with(Value *orig, Value *repl) {
    REPL_VALUE(m_val)
} // clang-format on

void CastInst::set_op(CastOp op) {
    m_op = op;
}

CompareInst::CompareInst(BasicBlock *parent, CompareOp op, Value *lhs, Value *rhs)
    : Instruction(KIND, parent), m_op(op), m_lhs(lhs), m_rhs(rhs) {
    m_lhs->add_user(this);
    m_rhs->add_user(this);
}

CompareInst::~CompareInst() {
    if (m_lhs != nullptr) {
        m_lhs->remove_user(this);
    }
    if (m_rhs != nullptr) {
        m_rhs->remove_user(this);
    }
}

void CompareInst::accept(Visitor *visitor) {
    visitor->visit(this);
}

// TODO: Figure out what's breaking clang-format here.
// clang-format off
void CompareInst::replace_uses_of_with(Value *orig, Value *repl) {
    REPL_VALUE(m_lhs)
    REPL_VALUE(m_rhs)
} // clang-format on

CondBranchInst::CondBranchInst(BasicBlock *parent, Value *cond, BasicBlock *true_dst, BasicBlock *false_dst)
    : Instruction(KIND, parent), m_cond(cond), m_true_dst(true_dst), m_false_dst(false_dst) {
    m_cond->add_user(this);
    m_true_dst->add_user(this);
    m_false_dst->add_user(this);
}

CondBranchInst::~CondBranchInst() {
    if (m_cond != nullptr) {
        m_cond->remove_user(this);
    }
    if (m_true_dst != nullptr) {
        m_true_dst->remove_user(this);
    }
    if (m_false_dst != nullptr) {
        m_false_dst->remove_user(this);
    }
}

void CondBranchInst::accept(Visitor *visitor) {
    visitor->visit(this);
}

// TODO: Figure out what's breaking clang-format here.
// clang-format off
void CondBranchInst::replace_uses_of_with(Value *orig, Value *repl) {
    REPL_VALUE(m_cond)
    REPL_VALUE(m_true_dst)
    REPL_VALUE(m_false_dst)
} // clang-format on

CopyInst::CopyInst(BasicBlock *parent, Value *dst, Value *src, Value *len)
    : Instruction(KIND, parent), m_dst(dst), m_src(src), m_len(len) {
    m_dst->add_user(this);
    m_src->add_user(this);
    m_len->add_user(this);
}

CopyInst::~CopyInst() {
    if (m_dst != nullptr) {
        m_dst->remove_user(this);
    }
    if (m_src != nullptr) {
        m_src->remove_user(this);
    }
    if (m_len != nullptr) {
        m_len->remove_user(this);
    }
}

void CopyInst::accept(Visitor *visitor) {
    visitor->visit(this);
}

// TODO: Figure out what's breaking clang-format here.
// clang-format off
void CopyInst::replace_uses_of_with(Value *orig, Value *repl) {
    REPL_VALUE(m_dst)
    REPL_VALUE(m_src)
    REPL_VALUE(m_len)
}
// clang-format on

InlineAsmInst::InlineAsmInst(BasicBlock *parent, std::string instruction, std::vector<std::string> &&clobbers,
                             std::vector<std::pair<std::string, Value *>> &&inputs,
                             std::vector<std::pair<std::string, Value *>> &&outputs)
    : Instruction(KIND, parent), m_instruction(std::move(instruction)), m_clobbers(std::move(clobbers)),
      m_inputs(std::move(inputs)), m_outputs(std::move(outputs)) {
    for (auto &[input, value] : m_inputs) {
        value->add_user(this);
    }
    for (auto &[output, value] : m_outputs) {
        value->add_user(this);
    }
}

InlineAsmInst::~InlineAsmInst() {
    for (auto &[input, value] : m_inputs) {
        if (value != nullptr) {
            value->remove_user(this);
        }
    }
    for (auto &[output, value] : m_outputs) {
        if (value != nullptr) {
            value->remove_user(this);
        }
    }
}

void InlineAsmInst::accept(Visitor *visitor) {
    visitor->visit(this);
}

void InlineAsmInst::replace_uses_of_with(Value *orig, Value *repl) {
    for (auto &[input, value] : m_inputs) {
        REPL_VALUE(value)
    }
    for (auto &[output, value] : m_outputs) {
        REPL_VALUE(value)
    }
}

LeaInst::LeaInst(BasicBlock *parent, Value *ptr, std::vector<Value *> &&indices)
    : Instruction(KIND, parent), m_ptr(ptr), m_indices(std::move(indices)) {
    m_ptr->add_user(this);
    for (auto *index : m_indices) {
        index->add_user(this);
    }
}

LeaInst::~LeaInst() {
    if (m_ptr != nullptr) {
        m_ptr->remove_user(this);
    }
    for (auto *index : m_indices) {
        if (index != nullptr) {
            index->remove_user(this);
        }
    }
}

void LeaInst::accept(Visitor *visitor) {
    visitor->visit(this);
}

void LeaInst::replace_uses_of_with(Value *orig, Value *repl) {
    REPL_VALUE(m_ptr)
    for (auto *&index : m_indices) {
        REPL_VALUE(index)
    }
}

LoadInst::LoadInst(BasicBlock *parent, Value *ptr) : Instruction(KIND, parent), m_ptr(ptr) {
    m_ptr->add_user(this);
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

PhiInst::PhiInst(BasicBlock *parent)
    : Instruction(KIND, parent) {}

PhiInst::~PhiInst() {
    for (auto [block, value] : m_incoming) {
        ASSERT(block != nullptr);
        block->remove_user(this);
        if (value != nullptr) {
            value->remove_user(this);
        }
    }
}

void PhiInst::accept(Visitor *visitor) {
    visitor->visit(this);
}

void PhiInst::add_incoming(BasicBlock *block, Value *value) {
    block->add_user(this);
    // TODO: Proper undef value.
    if (value != nullptr) {
        value->add_user(this);
    }
    m_incoming[block] = value;
}

void PhiInst::remove_incoming(BasicBlock *) {
    // TODO: Implement.
    ENSURE_NOT_REACHED();
}

void PhiInst::replace_uses_of_with(Value *orig, Value *repl) {
    // TODO: Can possibly be optimised, we can break early after replacing a block.
    for (auto it = m_incoming.begin(); it != m_incoming.end();) {
        auto *block = it->first;
        auto *&value = m_incoming[block];
        REPL_VALUE(value)
        if (block != orig) {
            ++it;
            continue;
        }
        block->remove_user(this);
        if (value != nullptr) {
            value->remove_user(this);
        }
        it = m_incoming.erase(it);
        if (repl != nullptr) {
            repl->add_user(this);
            if (value != nullptr) {
                value->add_user(this);
            }
            m_incoming.emplace(repl->as<BasicBlock>(), value);
        }
    }
}

StoreInst::StoreInst(BasicBlock *parent, Value *ptr, Value *val) : Instruction(KIND, parent), m_ptr(ptr), m_val(val) {
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

RetInst::RetInst(BasicBlock *parent, Value *val)
    : Instruction(KIND, parent), m_val(val) {
    if (m_val != nullptr) {
        m_val->add_user(this);
    }
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

} // namespace ir
