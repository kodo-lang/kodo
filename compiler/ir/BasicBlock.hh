#pragma once

#include <ir/Instruction.hh>
#include <ir/Value.hh>
#include <support/List.hh>
#include <support/ListNode.hh>

#include <concepts>

namespace ir {

class Function;

class BasicBlock : public Value, public ListNode {
    Function *m_parent{nullptr};
    // A linked list is used here to allow the insertion/removal of instructions whilst iterating.
    List<Instruction> m_instructions;

public:
    static constexpr auto KIND = ValueKind::BasicBlock;
    using iterator = decltype(m_instructions)::iterator;
    iterator begin() const { return m_instructions.begin(); }
    iterator end() const { return m_instructions.end(); }

    BasicBlock() : Value(KIND) {}

    /// @brief Inserts a new instruction `Inst` before `position`.
    /// @details Constructs a new instruction `Inst`, forwarding `args` to the constructor. Inserts the instruction
    /// before the position specified by `position`.
    /// @param position The position to insert the instruction before.
    /// @param args The arguments to be forwarded to the instruction's constructor.
    /// @return A pointer to the newly created instruction.
    /// @see prepend
    /// @see append
    template <typename Inst, typename... Args>
    Inst *insert(iterator position, Args &&... args) requires std::derived_from<Inst, Instruction>;

    /// @brief Prepends a new instruction `Inst` to the beginning of the block.
    /// @details Constructs a new instruction `Inst`, forwarding `args` to the constructor. Prepends the instruction to
    /// the beginning of the block.
    /// @param args The arguments to be forwarded to the instruction's constructor.
    /// @return A pointer to the newly created intruction.
    /// @see insert
    /// @see append
    template <typename Inst, typename... Args>
    Inst *prepend(Args &&... args) requires std::derived_from<Inst, Instruction>;

    /// @brief Appends a new instruction `Inst` to the end of the block.
    /// @details Constructs a new instruction `Inst`, forwarding `args` to the constructor. Appends the instruction to
    /// the end of the block.
    /// @param args The arguments to be forwarded to the instruction's constructor.
    /// @return A pointer to the newly created intruction.
    /// @see insert
    /// @see prepend
    template <typename Inst, typename... Args>
    Inst *append(Args &&... args) requires std::derived_from<Inst, Instruction>;

    iterator position(const Instruction *inst) const;

    /// @return The iterator of the next instruction.
    iterator remove(Instruction *inst);

    bool has_parent() const;
    void set_parent(Function *parent);

    Function *parent() const { return m_parent; }
    Instruction *terminator() const;
};

template <typename Inst, typename... Args>
Inst *BasicBlock::insert(iterator position, Args &&... args) requires std::derived_from<Inst, Instruction> {
    return m_instructions.emplace<Inst>(position, this, std::forward<Args>(args)...);
}

template <typename Inst, typename... Args>
Inst *BasicBlock::prepend(Args &&... args) requires std::derived_from<Inst, Instruction> {
    return insert<Inst>(m_instructions.begin(), std::forward<Args>(args)...);
}

template <typename Inst, typename... Args>
Inst *BasicBlock::append(Args &&... args) requires std::derived_from<Inst, Instruction> {
    return insert<Inst>(m_instructions.end(), std::forward<Args>(args)...);
}

} // namespace ir
