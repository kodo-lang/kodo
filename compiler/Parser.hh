#pragma once

#include <Token.hh>

class AstNode;
class Lexer;

class Parser {
    Lexer *const m_lexer;

    bool consume(TokenKind kind);
    Token expect(TokenKind kind);

    AstNode *parse_expr();
    AstNode *parse_stmt();

public:
    explicit Parser(Lexer *lexer) : m_lexer(lexer) {}

    AstNode *parse();
};
