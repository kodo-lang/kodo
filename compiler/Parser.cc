#include <Parser.hh>

#include <Lexer.hh>
#include <Token.hh>
#include <Type.hh>
#include <support/Stack.hh>

#include <fmt/color.h>
#include <fmt/core.h>

namespace {

enum class Op {
    // Binary operators.
    Add,
    Sub,
    Mul,
    Div,

    // Unary operators.
    AddressOf,
    Deref,

    // Other operators.
    Assign,
};

constexpr int precedence(Op op) {
    switch (op) {
    case Op::Assign:
        return 0;
    case Op::Add:
    case Op::Sub:
        return 1;
    case Op::Mul:
    case Op::Div:
        return 2;
    case Op::AddressOf:
    case Op::Deref:
        return 3;
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

ast::Node *create_expr(Op op, Stack<ast::Node *> *operands) {
    auto *rhs = operands->pop();
    switch (op) {
    case Op::AddressOf:
        return new ast::UnaryExpr(ast::UnaryOp::AddressOf, rhs);
    case Op::Deref:
        return new ast::UnaryExpr(ast::UnaryOp::Deref, rhs);
    default:
        break;
    }
    auto *lhs = operands->pop();
    switch (op) {
    case Op::Add:
        return new ast::BinExpr(ast::BinOp::Add, lhs, rhs);
    case Op::Sub:
        return new ast::BinExpr(ast::BinOp::Sub, lhs, rhs);
    case Op::Mul:
        return new ast::BinExpr(ast::BinOp::Mul, lhs, rhs);
    case Op::Div:
        return new ast::BinExpr(ast::BinOp::Div, lhs, rhs);
    case Op::Assign:
        return new ast::AssignExpr(lhs, rhs);
    default:
        assert(false);
    }
}

template <typename FmtString, typename... Args>
[[noreturn]] void error(const FmtString &fmt, const Args &... args) {
    auto formatted = fmt::format(fmt, args...);
    fmt::print(fmt::fg(fmt::color::orange_red), "parser: {}\n", formatted);
    abort();
}

} // namespace

std::optional<Token> Parser::consume(TokenKind kind) {
    if (m_lexer->peek().kind == kind) {
        return m_lexer->next();
    }
    return std::nullopt;
}

Token Parser::expect(TokenKind kind) {
    auto next = m_lexer->next();
    if (next.kind != kind) {
        error("expected {} but got {} on line {}", tok_str(kind), tok_str(next), m_lexer->line());
    }
    return next;
}

ast::CallExpr *Parser::parse_call_expr(std::string name) {
    auto *call_expr = new ast::CallExpr(std::move(name));
    m_lexer->next();
    while (m_lexer->has_next()) {
        if (m_lexer->peek().kind == TokenKind::RParen) {
            break;
        }
        call_expr->add_arg(parse_expr());
        consume(TokenKind::Comma);
    }
    expect(TokenKind::RParen);
    return call_expr;
}

ast::Node *Parser::parse_expr() {
    Stack<ast::Node *> operands;
    Stack<Op> operators;
    bool keep_parsing = true;
    bool last_was_operator = true;
    while (keep_parsing) {
        auto token = m_lexer->peek();
        auto op1 = [&token, last_was_operator]() -> std::optional<Op> {
            switch (token.kind) {
            case TokenKind::Add:
                return Op::Add;
            case TokenKind::Sub:
                return Op::Sub;
            case TokenKind::Mul:
                return last_was_operator ? Op::Deref : Op::Mul;
            case TokenKind::Div:
                return Op::Div;
            case TokenKind::Ampersand:
                return Op::AddressOf;
            case TokenKind::Eq:
                return Op::Assign;
            default:
                return std::nullopt;
            }
        }();
        last_was_operator = op1.has_value();
        if (!op1) {
            switch (token.kind) {
            case TokenKind::Identifier: {
                // TODO: Handle calls properly.
                auto name = std::move(std::get<std::string>(m_lexer->next().data));
                if (m_lexer->peek().kind == TokenKind::LParen) {
                    operands.push(parse_call_expr(std::move(name)));
                } else {
                    operands.push(new ast::Symbol(std::move(name)));
                }
                break;
            }
            case TokenKind::NumLit:
                operands.push(new ast::NumLit(std::get<std::uint64_t>(m_lexer->next().data)));
                break;
            default:
                keep_parsing = false;
                break;
            }
            continue;
        }

        m_lexer->next();
        while (!operators.empty()) {
            auto op2 = operators.peek();
            if (compare_op(*op1, op2) >= 0) {
                break;
            }
            auto op = operators.pop();
            operands.push(create_expr(op, &operands));
        }
        operators.push(*op1);
    }

    while (!operators.empty()) {
        auto op = operators.pop();
        operands.push(create_expr(op, &operands));
    }

    if (operands.size() != 1) {
        error("unfinished expression on line {}", m_lexer->line());
    }
    return operands.pop();
}

void Parser::parse_stmt(ast::FunctionDecl *func) {
    switch (m_lexer->peek().kind) {
    case TokenKind::Return:
        consume(TokenKind::Return);
        func->add_stmt<ast::RetStmt>(parse_expr());
        break;
    case TokenKind::Var: {
        consume(TokenKind::Var);
        auto name = std::move(std::get<std::string>(expect(TokenKind::Identifier).data));
        expect(TokenKind::Colon);
        const auto *type = parse_type();
        auto *stmt = func->add_stmt<ast::DeclStmt>(std::move(name), consume(TokenKind::Eq) ? parse_expr() : nullptr);
        stmt->set_type(type);
        break;
    }
    default:
        func->add_stmt(parse_expr());
        break;
    }
}

const Type *Parser::parse_type() {
    // TODO: TokenKind::Mul misleading.
    int pointer_levels = 0;
    while (consume(TokenKind::Mul)) {
        pointer_levels++;
    }
    auto base = std::get<std::string>(expect(TokenKind::Identifier).data);
    const Type *base_type = nullptr;
    if (base.starts_with('i')) {
        base_type = IntType::get(std::stoi(base.substr(1)));
    }

    if (base_type == nullptr) {
        error("invalid type {}", base);
    }

    const auto *type = base_type;
    for (int i = 0; i < pointer_levels; i++) {
        type = PointerType::get(type);
    }
    return type;
}

std::unique_ptr<ast::Root> Parser::parse() {
    auto root = std::make_unique<ast::Root>();
    while (m_lexer->has_next() && m_lexer->peek().kind != TokenKind::Eof) {
        bool externed = consume(TokenKind::Extern).has_value();
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
