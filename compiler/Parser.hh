#pragma once

#include <Token.hh>
#include <ast/Nodes.hh>

#include <cassert>
#include <memory>
#include <optional>

class Lexer;
class Type;

class Parser {
    Lexer *const m_lexer;

    std::optional<Token> consume(TokenKind kind);
    Token expect(TokenKind kind);

    ast::CallExpr *parse_call_expr(std::string name);
    ast::Node *parse_expr();
    void parse_stmt(ast::FunctionDecl *);
    const Type *parse_type();

public:
    explicit Parser(Lexer *lexer) : m_lexer(lexer) {}

    std::unique_ptr<ast::Root> parse();
};
