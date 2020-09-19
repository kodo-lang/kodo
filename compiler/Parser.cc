#include <Parser.hh>

#include <Ast.hh>
#include <Lexer.hh>
#include <Stack.hh>
#include <Token.hh>

#include <cassert>
#include <type_traits>

namespace {

// clang-format off
template <typename T> requires (!std::is_same_v<bool, T>)
class Result {
    // clang-format on
    bool m_present;
    T m_value;

public:
    constexpr Result(bool present) : m_present(present) { assert(!present); }
    constexpr Result(T value) : m_present(true), m_value(value) {}

    constexpr operator bool() const { return m_present; }
    constexpr operator T() const { return m_value; }
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

} // namespace

AstNode *Parser::parse_expr() {
    Stack<AstNode *> operands;
    Stack<BinOp> operators;
    while (true) {
        auto token = m_lexer->peek();
        auto op1 = token_to_bin_op(token);
        if (!op1) {
            if (token.kind == TokenKind::NumLit) {
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

AstNode *Parser::parse() {
    assert(m_lexer->next().kind == TokenKind::Return);
    auto *expr = parse_expr();
    assert(m_lexer->next().kind == TokenKind::Semi);
    return new RetStmt(expr);
}
