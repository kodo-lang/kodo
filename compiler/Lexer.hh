#pragma once

#include <Token.hh>

class CharStream;

class Lexer {
    CharStream *const m_stream;
    int m_line{1};

    bool m_peek_ready{false};
    Token m_peek_token{};

    Token next_token();

public:
    explicit Lexer(CharStream *stream) : m_stream(stream) {}

    bool has_next();
    Token next();
    Token peek();

    int line() const { return m_line; }
};
