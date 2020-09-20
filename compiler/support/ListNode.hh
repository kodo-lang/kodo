#pragma once

class ListNode {
    ListNode *m_prev{nullptr};
    ListNode *m_next{nullptr};

public:
    virtual ~ListNode() = default;

    void set_prev(ListNode *prev) { m_prev = prev; }
    void set_next(ListNode *next) { m_next = next; }

    ListNode *prev() const { return m_prev; }
    ListNode *next() const { return m_next; }
};
