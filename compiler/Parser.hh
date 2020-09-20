#pragma once

#include <Token.hh>

#include <cassert>

class AstNode;
class Lexer;
class Type;

// clang-format off
template <typename T>
class Result {
    // clang-format on
    bool m_present;
    T m_value;

public:
    constexpr Result(bool present) : m_present(present) { assert(!present); }
    constexpr Result(T value) : m_present(true), m_value(value) {}

    constexpr operator bool() const { return m_present; }
    constexpr operator T() const { return m_value; }
    constexpr T *operator->() { return &m_value; }
};

class Parser {
    Lexer *const m_lexer;

    Result<Token> consume(TokenKind kind);
    Token expect(TokenKind kind);

    AstNode *parse_expr();
    AstNode *parse_stmt();
    Type *parse_type();

public:
    explicit Parser(Lexer *lexer) : m_lexer(lexer) {}

    AstNode *parse();
};
