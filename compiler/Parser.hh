#pragma once

#include <Token.hh>

class AstNode;
class Lexer;

class Parser {
    Lexer *const m_lexer;

    void expect(TokenKind kind);

    AstNode *parse_expr();

public:
    explicit Parser(Lexer *lexer) : m_lexer(lexer) {}

    AstNode *parse();
};
