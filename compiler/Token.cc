#include <Token.hh>

#include <support/Assert.hh>

std::string tok_str(TokenKind kind) {
    switch (kind) {
    case TokenKind::Add:
        return "+";
    case TokenKind::Ampersand:
        return "&";
    case TokenKind::Arrow:
        return "->";
    case TokenKind::Asm:
        return "asm";
    case TokenKind::Cast:
        return "cast";
    case TokenKind::Clobber:
        return "clobber";
    case TokenKind::Colon:
        return ":";
    case TokenKind::Comma:
        return ",";
    case TokenKind::Const:
        return "const";
    case TokenKind::Div:
        return "/";
    case TokenKind::Dot:
        return ".";
    case TokenKind::DoubleColon:
        return "::";
    case TokenKind::Eof:
        return "eof";
    case TokenKind::Eq:
        return "=";
    case TokenKind::Extern:
        return "extern";
    case TokenKind::Fn:
        return "fn";
    case TokenKind::GreaterThan:
        return ">";
    case TokenKind::Identifier:
        return "identifier";
    case TokenKind::If:
        return "if";
    case TokenKind::Import:
        return "import";
    case TokenKind::In:
        return "in";
    case TokenKind::LBrace:
        return "{";
    case TokenKind::LessThan:
        return "<";
    case TokenKind::Let:
        return "let";
    case TokenKind::LParen:
        return "(";
    case TokenKind::Mul:
        return "*";
    case TokenKind::Mut:
        return "mut";
    case TokenKind::NumLit:
        return "number";
    case TokenKind::Output:
        return "output";
    case TokenKind::Return:
        return "return";
    case TokenKind::RBrace:
        return "}";
    case TokenKind::RParen:
        return ")";
    case TokenKind::Semi:
        return ";";
    case TokenKind::StringLit:
        return "string";
    case TokenKind::Struct:
        return "struct";
    case TokenKind::Sub:
        return "-";
    case TokenKind::This:
        return "this";
    case TokenKind::Type:
        return "type";
    case TokenKind::Var:
        return "var";
    default:
        ENSURE_NOT_REACHED();
    }
}

std::string tok_str(const Token &token) {
    switch (token.kind) {
    case TokenKind::Identifier:
    case TokenKind::StringLit:
        return '"' + std::get<std::string>(token.data) + '"';
    case TokenKind::NumLit:
        return std::to_string(std::get<std::uint64_t>(token.data));
    default:
        return tok_str(token.kind);
    }
}
