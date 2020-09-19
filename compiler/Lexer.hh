#pragma once

#include <Token.hh>

class CharStream;

class Lexer {
    CharStream *const m_stream;
    int m_line{0};

public:
    explicit Lexer(CharStream *stream) : m_stream(stream) {}

    Token next();
};
