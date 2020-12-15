#pragma once

#include <ir/Value.hh>
#include <support/ListNode.hh>

#include <string>

namespace ir {

class FunctionType;

class Prototype : public Value, public ListNode {
    const bool m_externed;

public:
    static constexpr auto KIND = ValueKind::Prototype;

    Prototype(bool externed, std::string name, const FunctionType *type);
    Prototype(const Prototype &) = delete;
    Prototype(Prototype &&) = delete;
    ~Prototype() override = default;

    Prototype &operator=(const Prototype &) = delete;
    Prototype &operator=(Prototype &&) = delete;

    bool externed() const { return m_externed; }
    const Type *return_type() const;
    const std::vector<const Type *> &params() const;
};

} // namespace ir
