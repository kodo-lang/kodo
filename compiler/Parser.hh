#pragma once

#include <Token.hh>
#include <ast/Nodes.hh>
#include <support/Box.hh>

#include <optional>

class Lexer;

class Parser {
    Lexer *const m_lexer;

    std::optional<Token> consume(TokenKind kind);
    Token expect(TokenKind kind);

    ast::AsmExpr *parse_asm_expr();
    ast::CallExpr *parse_call_expr(const ast::Symbol *name);
    ast::CastExpr *parse_cast_expr();
    ast::ConstructExpr *parse_construct_expr(const ast::Symbol *name);
    ast::Symbol *parse_symbol();
    ast::Node *parse_expr();
    void parse_stmt(ast::Block *);
    ast::Node *parse_type();
    ast::Block *parse_block();

public:
    explicit Parser(Lexer *lexer) : m_lexer(lexer) {}

    Box<ast::Root> parse();
};
