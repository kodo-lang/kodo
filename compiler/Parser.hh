#pragma once

class AstNode;
class Lexer;

class Parser {
    Lexer *const m_lexer;

    AstNode *parse_expr();

public:
    explicit Parser(Lexer *lexer) : m_lexer(lexer) {}

    AstNode *parse();
};
