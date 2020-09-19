#include <Parser.hh>

#include <Ast.hh>
#include <Lexer.hh>
#include <Stack.hh>
#include <Token.hh>

#include <fmt/color.h>
#include <fmt/core.h>

#include <cassert>

namespace {

constexpr Result<BinOp> token_to_bin_op(const Token &token) {
    switch (token.kind) {
    case TokenKind::Add:
        return BinOp::Add;
    case TokenKind::Sub:
        return BinOp::Sub;
    case TokenKind::Mul:
        return BinOp::Mul;
    case TokenKind::Div:
        return BinOp::Div;
    default:
        return false;
    }
}

constexpr int precedence(BinOp op) {
    switch (op) {
    case BinOp::Add:
    case BinOp::Sub:
        return 1;
    case BinOp::Mul:
    case BinOp::Div:
        return 2;
    }
}

constexpr int compare_op(BinOp op1, BinOp op2) {
    int p1 = precedence(op1);
    int p2 = precedence(op2);
    if (p1 == p2) {
        return 0;
    }
    return p1 > p2 ? 1 : -1;
}

template <typename FmtString, typename... Args>
[[noreturn]] void error(const FmtString &fmt, const Args &... args) {
    auto formatted = fmt::format(fmt, args...);
    fmt::print(fmt::fg(fmt::color::orange_red), "parser: {}\n", formatted);
    abort();
}

} // namespace

Result<Token> Parser::consume(TokenKind kind) {
    if (m_lexer->peek().kind == kind) {
        return m_lexer->next();
    }
    return false;
}

Token Parser::expect(TokenKind kind) {
    auto next = m_lexer->next();
    if (next.kind != kind) {
        error("expected {} but got {} on line {}", tok_str(kind), tok_str(next), m_lexer->line());
    }
    return next;
}

AstNode *Parser::parse_expr() {
    Stack<AstNode *> operands;
    Stack<BinOp> operators;
    while (true) {
        auto token = m_lexer->peek();
        auto op1 = token_to_bin_op(token);
        if (!op1) {
            if (token.kind == TokenKind::Identifier) {
                m_lexer->next();
                operands.push(new VarExpr(token.text));
                continue;
            } else if (token.kind == TokenKind::NumLit) {
                m_lexer->next();
                operands.push(new NumLit(token.num));
                continue;
            }
            break;
        }

        m_lexer->next();
        while (!operators.empty()) {
            auto op2 = operators.peek();
            if (compare_op(op1, op2) > 0) {
                break;
            }
            auto *rhs = operands.pop();
            auto *lhs = operands.pop();
            operands.push(new BinExpr(operators.pop(), lhs, rhs));
        }
        operators.push(op1);
    }

    while (!operators.empty()) {
        auto *rhs = operands.pop();
        auto *lhs = operands.pop();
        operands.push(new BinExpr(operators.pop(), lhs, rhs));
    }

    assert(operands.size() == 1);
    return operands.pop();
}

AstNode *Parser::parse_stmt() {
    if (auto name = consume(TokenKind::Identifier)) {
        expect(TokenKind::Eq);
        return new AssignStmt(name->text, parse_expr());
    }
    if (consume(TokenKind::Return)) {
        return new RetStmt(parse_expr());
    }
    if (consume(TokenKind::Var)) {
        const char *name = expect(TokenKind::Identifier).text;
        expect(TokenKind::Colon);
        expect(TokenKind::Identifier);
        if (consume(TokenKind::Eq)) {
            return new DeclStmt(name, parse_expr());
        }
        return new DeclStmt(name, nullptr);
    }
    error("expected stmt but got {} on line {}", tok_str(m_lexer->next()), m_lexer->line());
}

AstNode *Parser::parse() {
    expect(TokenKind::Fn);
    const char *name = expect(TokenKind::Identifier).text;
    expect(TokenKind::LParen);
    expect(TokenKind::RParen);
    expect(TokenKind::Arrow);
    expect(TokenKind::Identifier);
    expect(TokenKind::LBrace);
    auto *func = new FunctionDecl(name);
    while (m_lexer->has_next()) {
        if (m_lexer->peek().kind == TokenKind::RBrace) {
            break;
        }
        func->add_stmt(parse_stmt());
        expect(TokenKind::Semi);
    }
    expect(TokenKind::RBrace);
    return func;
}
