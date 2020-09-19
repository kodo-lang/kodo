#include <Token.hh>

std::string tok_str(const Token &token) {
    switch (token.kind) {
    case TokenKind::Add:
        return "+";
    case TokenKind::Eof:
        return "eof";
    case TokenKind::NumLit:
        return std::to_string(token.num);
    }
}
