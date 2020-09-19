#include <CharStream.hh>

#include <cassert>

bool CharStream::has_next() {
    return peek() != EOF;
}

char CharStream::peek() {
    return static_cast<char>(m_stream->peek());
}

char CharStream::next() {
    assert(has_next());
    return static_cast<char>(m_stream->get());
}
