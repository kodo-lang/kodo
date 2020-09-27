#include <Parser.hh>

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
        ast::BinOp bin_op;
        ast::UnaryOp unary_op;
    };
};

constexpr Result<ast::BinOp> token_to_bin_op(const Token &token) {
    switch (token.kind) {
    case TokenKind::Add:
        return ast::BinOp::Add;
    case TokenKind::Sub:
        return ast::BinOp::Sub;
    case TokenKind::Mul:
        return ast::BinOp::Mul;
    case TokenKind::Div:
        return ast::BinOp::Div;
    default:
        return false;
    }
}

constexpr int precedence(Op op) {
    if (op.unary) {
        return 3;
    }
    switch (op.bin_op) {
    case ast::BinOp::Add:
    case ast::BinOp::Sub:
        return 1;
    case ast::BinOp::Mul:
    case ast::BinOp::Div:
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

ast::Node *Parser::parse_expr() {
    Stack<ast::Node *> operands;
    Stack<Op> operators;
    bool last_was_op = true;
    while (true) {
        if (operands.empty() && consume(TokenKind::Ampersand)) {
            operands.push(new ast::UnaryExpr(ast::UnaryOp::AddressOf, parse_expr()));
            continue;
        }

        auto token = m_lexer->peek();
        auto bin_op = token_to_bin_op(token);
        if (!bin_op) {
            last_was_op = false;
            if (token.kind == TokenKind::Identifier) {
                m_lexer->next();
                if (m_lexer->peek().kind != TokenKind::LParen) {
                    operands.push(new ast::VarExpr(std::move(std::get<std::string>(token.data))));
                    continue;
                }
                m_lexer->next();
                auto *call_expr = new ast::CallExpr(std::move(std::get<std::string>(token.data)));
                while (m_lexer->has_next()) {
                    if (m_lexer->peek().kind == TokenKind::RParen) {
                        break;
                    }
                    call_expr->add_arg(parse_expr());
                    consume(TokenKind::Comma);
                }
                expect(TokenKind::RParen);
                operands.push(call_expr);
                continue;
            } else if (token.kind == TokenKind::NumLit) {
                m_lexer->next();
                operands.push(new ast::NumLit(std::get<std::uint64_t>(token.data)));
                continue;
            }
            break;
        }

        Op op1 = {false, bin_op};
        if (op1.bin_op == ast::BinOp::Mul && last_was_op) {
            op1.unary = true;
            op1.unary_op = ast::UnaryOp::Deref;
        }
        last_was_op = true;

        m_lexer->next();
        while (!operators.empty()) {
            auto op2 = operators.peek();
            if (compare_op(op1, op2) > 0 || op1.unary) {
                break;
            }
            auto op = operators.pop();
            auto *rhs = operands.pop();
            if (op.unary) {
                operands.push(new ast::UnaryExpr(op.unary_op, rhs));
            } else {
                auto *lhs = operands.pop();
                operands.push(new ast::BinExpr(op.bin_op, lhs, rhs));
            }
        }
        operators.push(op1);
    }

    while (!operators.empty()) {
        auto op = operators.pop();
        auto *rhs = operands.pop();
        if (op.unary) {
            operands.push(new ast::UnaryExpr(op.unary_op, rhs));
        } else {
            auto *lhs = operands.pop();
            operands.push(new ast::BinExpr(op.bin_op, lhs, rhs));
        }
    }

    if (operands.size() != 1) {
        error("unfinished expression on line {}", m_lexer->line());
    }
    return operands.pop();
}

void Parser::parse_stmt(ast::FunctionDecl *func) {
    switch (m_lexer->peek().kind) {
//    case TokenKind::Identifier: {
//        auto name = std::move(std::get<std::string>(expect(TokenKind::Identifier).data));
//        expect(TokenKind::Eq);
//        func->add_stmt<AssignStmt>(std::move(name), parse_expr());
//        break;
//    }
    case TokenKind::Return:
        consume(TokenKind::Return);
        func->add_stmt<ast::RetStmt>(parse_expr());
        break;
    case TokenKind::Var: {
        consume(TokenKind::Var);
        auto name = std::move(std::get<std::string>(expect(TokenKind::Identifier).data));
        expect(TokenKind::Colon);
        auto *type = parse_type();
        auto *stmt = func->add_stmt<ast::DeclStmt>(std::move(name), consume(TokenKind::Eq) ? parse_expr() : nullptr);
        stmt->set_type(type);
        break;
    }
    default:
        func->add_stmt(parse_expr());
//        error("expected stmt but got {} on line {}", tok_str(m_lexer->next()), m_lexer->line());
    }
}

Type *Parser::parse_type() {
    // TODO: TokenKind::Mul misleading.
    int pointer_levels = 0;
    while (consume(TokenKind::Mul)) {
        pointer_levels++;
    }
    auto base = std::get<std::string>(expect(TokenKind::Identifier).data);
    Type *base_type = nullptr;
    if (base.starts_with('i')) {
        base_type = IntType::get(std::stoi(base.substr(1)));
    }

    if (base_type == nullptr) {
        error("invalid type {}", base);
    }

    auto *type = base_type;
    for (int i = 0; i < pointer_levels; i++) {
        type = PointerType::get(type);
    }
    return type;
}

std::unique_ptr<ast::Root> Parser::parse() {
    auto root = std::make_unique<ast::Root>();
    while (m_lexer->has_next() && m_lexer->peek().kind != TokenKind::Eof) {
        bool externed = consume(TokenKind::Extern);
        expect(TokenKind::Fn);
        auto name = expect(TokenKind::Identifier);
        auto *func = root->add_function(std::move(std::get<std::string>(name.data)), externed);
        expect(TokenKind::LParen);
        while (m_lexer->peek().kind != TokenKind::RParen) {
            auto arg_name = expect(TokenKind::Identifier);
            expect(TokenKind::Colon);
            auto *arg = func->add_arg(std::move(std::get<std::string>(arg_name.data)));
            arg->set_type(parse_type());
            consume(TokenKind::Comma);
        }
        expect(TokenKind::RParen);
        expect(TokenKind::Arrow);
        func->set_type(parse_type());
        if (externed) {
            expect(TokenKind::Semi);
            continue;
        }
        expect(TokenKind::LBrace);
        while (m_lexer->has_next()) {
            if (m_lexer->peek().kind == TokenKind::RBrace) {
                break;
            }
            parse_stmt(func);
            expect(TokenKind::Semi);
        }
        expect(TokenKind::RBrace);
    }
    return std::move(root);
}
