#pragma once

#include <ir/Constant.hh>
#include <ir/Value.hh>
#include <support/ListNode.hh>

namespace ir {

class GlobalVariable : public Value, public ListNode {
    const ir::Constant *const m_initialiser;

public:
    static constexpr auto KIND = ValueKind::GlobalVariable;

    explicit GlobalVariable(const ir::Constant *initialiser);
    GlobalVariable(const GlobalVariable &) = delete;
    GlobalVariable(GlobalVariable &&) = delete;
    ~GlobalVariable() override = default;

    GlobalVariable &operator=(const GlobalVariable &) = delete;
    GlobalVariable &operator=(GlobalVariable &&) = delete;

    const ir::Constant *initialiser() const { return m_initialiser; }
};

} // namespace ir
