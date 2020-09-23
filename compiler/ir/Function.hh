#pragma once

#include <ir/BasicBlock.hh>
#include <support/List.hh>
#include <support/ListNode.hh>

#include <string>
#include <utility>

class Function : public ListNode {
    const std::string m_name;
    List<BasicBlock> m_blocks;

public:
    using iterator = decltype(m_blocks)::iterator;
    iterator begin() const { return m_blocks.begin(); }
    iterator end() const { return m_blocks.end(); }

    explicit Function(std::string name) : m_name(std::move(name)) {}
    Function(const Function &) = delete;
    Function(Function &&) = delete;
    ~Function() override = default;

    Function &operator=(const Function &) = delete;
    Function &operator=(Function &&) = delete;

    BasicBlock *append_block();

    const std::string &name() const { return m_name; }
    BasicBlock *entry() const;
};
