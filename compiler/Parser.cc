#include <Parser.hh>

#include <Lexer.hh>
#include <Token.hh>
#include <support/Assert.hh>
#include <support/Error.hh>
#include <support/Stack.hh>

namespace {

enum class Op {
    // Binary arithmetic operators.
    Add,
    Sub,
    Mul,
    Div,

    // Binary comparison operators.
    LessThan,
    GreaterThan,

    // Unary operators.
    AddressOf,
    Deref,

    // Other operators.
    Assign,
    Member,
    MemberPtr,
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
    case Op::LessThan:
    case Op::GreaterThan:
        return 3;
    case Op::AddressOf:
    case Op::Deref:
        return 4;
    case Op::Member:
    case Op::MemberPtr:
        return 5;
    default:
        ENSURE_NOT_REACHED();
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
        return new ast::UnaryExpr(rhs->line(), ast::UnaryOp::AddressOf, rhs);
    case Op::Deref:
        return new ast::UnaryExpr(rhs->line(), ast::UnaryOp::Deref, rhs);
    default:
        break;
    }
    auto *lhs = operands->pop();
    switch (op) {
    case Op::Add:
        return new ast::BinExpr(rhs->line(), ast::BinOp::Add, lhs, rhs);
    case Op::Sub:
        return new ast::BinExpr(rhs->line(), ast::BinOp::Sub, lhs, rhs);
    case Op::Mul:
        return new ast::BinExpr(rhs->line(), ast::BinOp::Mul, lhs, rhs);
    case Op::Div:
        return new ast::BinExpr(rhs->line(), ast::BinOp::Div, lhs, rhs);
    case Op::LessThan:
        return new ast::BinExpr(rhs->line(), ast::BinOp::LessThan, lhs, rhs);
    case Op::GreaterThan:
        return new ast::BinExpr(rhs->line(), ast::BinOp::GreaterThan, lhs, rhs);
    case Op::Assign:
        return new ast::AssignExpr(rhs->line(), lhs, rhs);
    case Op::Member:
    case Op::MemberPtr:
        return new ast::MemberExpr(rhs->line(), lhs, rhs, op == Op::MemberPtr);
    default:
        ENSURE_NOT_REACHED();
    }
}

bool is_right_asc(Op op) {
    switch (op) {
    case Op::Member:
    case Op::MemberPtr:
        return true;
    default:
        return false;
    }
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
        print_error_and_abort("expected {} but got {} on line {}", tok_str(kind), tok_str(next), m_lexer->line());
    }
    return next;
}

ast::AsmExpr *Parser::parse_asm_expr() {
    expect(TokenKind::Asm);
    expect(TokenKind::LParen);
    auto *asm_expr =
        new ast::AsmExpr(m_lexer->line(), std::move(std::get<std::string>(expect(TokenKind::StringLit).data)));
    expect(TokenKind::Comma);
    while (m_lexer->has_next()) {
        if (m_lexer->peek().kind == TokenKind::RParen) {
            break;
        }
        enum class PartKind {
            Clobber,
            Input,
            Output,
        } part_kind;
        auto next = m_lexer->next();
        switch (next.kind) {
        case TokenKind::Clobber:
            part_kind = PartKind::Clobber;
            break;
        case TokenKind::In:
            part_kind = PartKind::Input;
            break;
        case TokenKind::Output:
            part_kind = PartKind::Output;
            break;
        default:
            print_error_and_abort("expected clobber or in on line {}", m_lexer->line());
        }
        expect(TokenKind::LParen);
        auto reg = std::move(std::get<std::string>(expect(TokenKind::StringLit).data));
        switch (part_kind) {
        case PartKind::Clobber:
            asm_expr->add_clobber(std::move(reg));
            break;
        case PartKind::Input:
            expect(TokenKind::Comma);
            asm_expr->add_input(std::move(reg), parse_expr());
            break;
        case PartKind::Output:
            expect(TokenKind::Comma);
            asm_expr->add_output(std::move(reg), parse_expr());
            break;
        }
        expect(TokenKind::RParen);
        // TODO: Be more strict on this.
        consume(TokenKind::Comma);
    }
    expect(TokenKind::RParen);
    return asm_expr;
}

ast::CallExpr *Parser::parse_call_expr(const ast::Symbol *name) {
    auto *call_expr = new ast::CallExpr(m_lexer->line(), name);
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

ast::CastExpr *Parser::parse_cast_expr() {
    expect(TokenKind::Cast);
    expect(TokenKind::LessThan);
    auto type = parse_type();
    expect(TokenKind::GreaterThan);
    expect(TokenKind::LParen);
    auto *expr = parse_expr();
    expect(TokenKind::RParen);
    return new ast::CastExpr(m_lexer->line(), std::move(type), expr);
}

ast::ConstructExpr *Parser::parse_construct_expr(const ast::Symbol *name) {
    ASSERT(name->parts().size() == 1);
    auto *construct_expr = new ast::ConstructExpr(m_lexer->line(), name->parts()[0]);
    m_lexer->next();
    while (m_lexer->has_next()) {
        if (m_lexer->peek().kind == TokenKind::RBrace) {
            break;
        }
        construct_expr->add_arg(parse_expr());
        consume(TokenKind::Comma);
    }
    expect(TokenKind::RBrace);
    return construct_expr;
}

ast::Symbol *Parser::parse_symbol() {
    std::vector<std::string> parts;
    parts.push_back(std::move(std::get<std::string>(expect(TokenKind::Identifier).data)));
    while (consume(TokenKind::DoubleColon)) {
        parts.push_back(std::move(std::get<std::string>(expect(TokenKind::Identifier).data)));
    }
    return new ast::Symbol(m_lexer->line(), std::move(parts));
}

ast::Node *Parser::parse_expr() {
    Stack<ast::Node *> operands;
    Stack<Op> operators;
    bool keep_parsing = true;
    bool last_was_operator = true;
    while (keep_parsing) {
        auto token = m_lexer->peek();
        if (token.kind == TokenKind::Asm) {
            operands.push(parse_asm_expr());
            continue;
        }
        if (token.kind == TokenKind::Cast) {
            operands.push(parse_cast_expr());
            continue;
        }

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
            case TokenKind::LessThan:
                return Op::LessThan;
            case TokenKind::GreaterThan:
                return Op::GreaterThan;
            case TokenKind::Ampersand:
                return Op::AddressOf;
            case TokenKind::Eq:
                return Op::Assign;
            case TokenKind::Dot:
                return Op::Member;
            case TokenKind::Arrow:
                return Op::MemberPtr;
            default:
                return std::nullopt;
            }
        }();
        last_was_operator = op1.has_value();
        if (!op1) {
            switch (token.kind) {
            case TokenKind::Identifier: {
                // TODO: Remove recursiveness.
                auto *symbol = parse_symbol();
                if (m_lexer->peek().kind == TokenKind::LParen) {
                    operands.push(parse_call_expr(symbol));
                } else if (m_lexer->peek().kind == TokenKind::LBrace) {
                    operands.push(parse_construct_expr(symbol));
                } else {
                    operands.push(symbol);
                }
                break;
            }
            case TokenKind::NumLit:
                operands.push(new ast::NumLit(m_lexer->line(), std::get<std::uint64_t>(m_lexer->next().data)));
                break;
            case TokenKind::StringLit:
                operands.push(new ast::StringLit(m_lexer->line(), std::get<std::string>(m_lexer->next().data)));
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
            int pred_cmp = compare_op(*op1, op2);
            if (pred_cmp > 0 || (pred_cmp == 0 && !is_right_asc(*op1))) {
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
        print_error_and_abort("unfinished expression on line {}", m_lexer->line());
    }
    return operands.pop();
}

void Parser::parse_stmt(ast::Block *block) {
    switch (m_lexer->peek().kind) {
    case TokenKind::If: {
        consume(TokenKind::If);
        expect(TokenKind::LParen);
        auto *expr = parse_expr();
        expect(TokenKind::RParen);
        block->add_stmt<ast::IfStmt>(m_lexer->line(), expr, parse_block());
        break;
    }
    case TokenKind::Let:
    case TokenKind::Var: {
        bool is_mutable = consume(TokenKind::Var).has_value();
        if (!is_mutable) {
            expect(TokenKind::Let);
        }
        auto name = std::move(std::get<std::string>(expect(TokenKind::Identifier).data));
        auto type = ast::Type::get_inferred();
        if (consume(TokenKind::Colon)) {
            type = parse_type();
        }
        const auto *init_val = consume(TokenKind::Eq) ? parse_expr() : nullptr;
        block->add_stmt<ast::DeclStmt>(m_lexer->line(), std::move(name), std::move(type), init_val, is_mutable);
        expect(TokenKind::Semi);
        break;
    }
    case TokenKind::Return:
        consume(TokenKind::Return);
        block->add_stmt<ast::RetStmt>(m_lexer->line(), parse_expr());
        expect(TokenKind::Semi);
        break;
    default:
        block->add_stmt(parse_expr());
        expect(TokenKind::Semi);
        break;
    }
}

ast::Type Parser::parse_type() {
    // TODO: TokenKind::Mul misleading.
    if (consume(TokenKind::Mul)) {
        bool is_mutable = consume(TokenKind::Mut).has_value();
        return ast::Type::get_pointer(parse_type(), is_mutable);
    }
    if (consume(TokenKind::Struct)) {
        std::vector<ast::StructField> fields;
        expect(TokenKind::LBrace);
        while (m_lexer->has_next()) {
            if (m_lexer->peek().kind == TokenKind::RBrace) {
                break;
            }
            auto name = expect(TokenKind::Identifier);
            expect(TokenKind::Colon);
            // TODO: Why doesn't this build with clang?
            // fields.emplace_back(std::move(std::get<std::string>(name.data)), parse_type());
            fields.push_back(ast::StructField{std::move(std::get<std::string>(name.data)), parse_type()});
            expect(TokenKind::Semi);
        }
        expect(TokenKind::RBrace);
        return ast::Type::get_struct(std::move(fields));
    }
    return ast::Type::get_base(std::move(std::get<std::string>(expect(TokenKind::Identifier).data)));
}

ast::Block *Parser::parse_block() {
    auto *block = new ast::Block(m_lexer->line());
    expect(TokenKind::LBrace);
    while (m_lexer->has_next()) {
        if (m_lexer->peek().kind == TokenKind::RBrace) {
            break;
        }
        parse_stmt(block);
    }
    expect(TokenKind::RBrace);
    return block;
}

std::unique_ptr<ast::Root> Parser::parse() {
    auto root = std::make_unique<ast::Root>();
    while (m_lexer->has_next() && m_lexer->peek().kind != TokenKind::Eof) {
        if (consume(TokenKind::Import)) {
            auto path = expect(TokenKind::StringLit);
            expect(TokenKind::Semi);
            root->add<ast::ImportStmt>(m_lexer->line(), std::move(std::get<std::string>(path.data)));
            continue;
        }
        if (consume(TokenKind::Type)) {
            auto name = expect(TokenKind::Identifier);
            expect(TokenKind::Eq);
            auto type = parse_type();
            expect(TokenKind::Semi);
            root->add<ast::TypeDecl>(m_lexer->line(), std::move(std::get<std::string>(name.data)), std::move(type));
            continue;
        }
        bool externed = consume(TokenKind::Extern).has_value();
        expect(TokenKind::Fn);
        auto *func = root->add<ast::FunctionDecl>(m_lexer->line(), parse_symbol(), externed);
        expect(TokenKind::LParen);
        while (m_lexer->peek().kind != TokenKind::RParen) {
            // TODO: `is_mutable = expect(TokenKind::Let, TokenKind::Var).kind == TokenKind::Var`.
            bool is_mutable = consume(TokenKind::Var).has_value();
            if (!is_mutable) {
                expect(TokenKind::Let);
            }
            auto arg_name = expect(TokenKind::Identifier);
            expect(TokenKind::Colon);
            func->add_arg(m_lexer->line(), std::move(std::get<std::string>(arg_name.data)), parse_type(), is_mutable);
            consume(TokenKind::Comma);
        }
        expect(TokenKind::RParen);
        if (consume(TokenKind::Colon)) {
            func->set_return_type(parse_type());
        } else {
            func->set_return_type(ast::Type::get_base("void"));
        }
        if (externed) {
            expect(TokenKind::Semi);
            continue;
        }
        func->set_block(parse_block());
    }
    return std::move(root);
}
