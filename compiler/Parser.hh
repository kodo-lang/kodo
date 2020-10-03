#pragma once

#include <Token.hh>
#include <ast/Nodes.hh>

#include <memory>
#include <optional>

class Lexer;
class Type;

class Parser {
    Lexer *const m_lexer;

    std::optional<Token> consume(TokenKind kind);
    Token expect(TokenKind kind);

    ast::CallExpr *parse_call_expr(std::string name);
    ast::CastExpr *parse_cast_expr();
    ast::Node *parse_expr();
    void parse_stmt(ast::Block *);
    const Type *parse_type();
    ast::Block *parse_block();

public:
    explicit Parser(Lexer *lexer) : m_lexer(lexer) {}

    std::unique_ptr<ast::Root> parse();
};
