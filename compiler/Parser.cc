#include <Parser.hh>

#include <Ast.hh>
#include <Lexer.hh>
#include <Stack.hh>
#include <Token.hh>
#include <Type.hh>

#include <fmt/color.h>
#include <fmt/core.h>

#include <cassert>

namespace {

struct Op {
    bool unary;
    union {
        BinOp bin_op;
        UnaryOp unary_op;
    };
};

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

constexpr int precedence(Op op) {
    if (op.unary) {
        return 3;
    }
    switch (op.bin_op) {
    case BinOp::Add:
    case BinOp::Sub:
        return 1;
    case BinOp::Mul:
    case BinOp::Div:
        return 2;
    }
}

constexpr int compare_op(Op op1, Op op2) {
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
    Stack<Op> operators;
    bool last_was_op = true;
    while (true) {
        if (operands.empty() && consume(TokenKind::Ampersand)) {
            operands.push(new UnaryExpr(UnaryOp::AddressOf, parse_expr()));
            continue;
        }

        auto token = m_lexer->peek();
        auto bin_op = token_to_bin_op(token);
        if (!bin_op) {
            last_was_op = false;
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

        Op op1 = {false, bin_op};
        if (op1.bin_op == BinOp::Mul && last_was_op) {
            op1.unary = true;
            op1.unary_op = UnaryOp::Deref;
        }
        last_was_op = true;

        m_lexer->next();
        while (!operators.empty()) {
            auto op2 = operators.peek();
            if (compare_op(op1, op2) > 0) {
                break;
            }
            auto op = operators.pop();
            auto *rhs = operands.pop();
            if (op.unary) {
                operands.push(new UnaryExpr(op.unary_op, rhs));
            } else {
                auto *lhs = operands.pop();
                operands.push(new BinExpr(op.bin_op, lhs, rhs));
            }
        }
        operators.push(op1);
    }

    while (!operators.empty()) {
        auto op = operators.pop();
        auto *rhs = operands.pop();
        if (op.unary) {
            operands.push(new UnaryExpr(op.unary_op, rhs));
        } else {
            auto *lhs = operands.pop();
            operands.push(new BinExpr(op.bin_op, lhs, rhs));
        }
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
        auto *type = parse_type();
        if (consume(TokenKind::Eq)) {
            auto *stmt = new DeclStmt(name, parse_expr());
            stmt->set_type(type);
            return stmt;
        }
        auto *stmt = new DeclStmt(name, nullptr);
        stmt->set_type(type);
        return stmt;
    }
    error("expected stmt but got {} on line {}", tok_str(m_lexer->next()), m_lexer->line());
}

Type *Parser::parse_type() {
    // TODO: TokenKind::Mul misleading.
    int pointer_levels = 0;
    while (consume(TokenKind::Mul)) {
        pointer_levels++;
    }
    std::string base = expect(TokenKind::Identifier).text;
    Type *base_type = nullptr;
    if (base.starts_with('i')) {
        base_type = new IntType(std::stoi(base.substr(1)));
    }

    if (base_type == nullptr) {
        error("invalid type {}", base);
    }

    auto *type = base_type;
    for (int i = 0; i < pointer_levels; i++) {
        type = new PointerType(type);
    }
    return type;
}

AstNode *Parser::parse() {
    expect(TokenKind::Fn);
    const char *name = expect(TokenKind::Identifier).text;
    auto *func = new FunctionDecl(name);
    expect(TokenKind::LParen);
    while (m_lexer->peek().kind != TokenKind::RParen) {
        const char *arg_name = expect(TokenKind::Identifier).text;
        expect(TokenKind::Colon);
        auto *arg = new FunctionArg(arg_name);
        arg->set_type(parse_type());
        func->add_arg(arg);
        consume(TokenKind::Comma);
    }
    expect(TokenKind::RParen);
    expect(TokenKind::Arrow);
    expect(TokenKind::Identifier);
    expect(TokenKind::LBrace);
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
