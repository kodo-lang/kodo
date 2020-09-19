#include <Token.hh>

std::string tok_str(TokenKind kind) {
    switch (kind) {
    case TokenKind::Add:
        return "+";
    case TokenKind::Div:
        return "/";
    case TokenKind::Eof:
        return "eof";
    case TokenKind::Mul:
        return "*";
    case TokenKind::NumLit:
        return "number";
    case TokenKind::Return:
        return "return";
    case TokenKind::Semi:
        return ";";
    case TokenKind::Sub:
        return "-";
    }
}

std::string tok_str(const Token &token) {
    switch (token.kind) {
    case TokenKind::NumLit:
        return std::to_string(token.num);
    default:
        return tok_str(token.kind);
    }
}
