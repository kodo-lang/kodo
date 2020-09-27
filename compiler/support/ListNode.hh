#pragma once

class ListNode {
    mutable ListNode *m_prev{nullptr};
    mutable ListNode *m_next{nullptr};

public:
    virtual ~ListNode() = default;

    void set_prev(const ListNode *prev) const { m_prev = const_cast<ListNode *>(prev); }
    void set_next(const ListNode *next) const { m_next = const_cast<ListNode *>(next); }

    ListNode *prev() const { return m_prev; }
    ListNode *next() const { return m_next; }
};
