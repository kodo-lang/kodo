#include <Lexer.hh>

#include <CharStream.hh>

#include <cctype>
#include <stdexcept>
#include <string>

Token Lexer::next() {
    while (std::isspace(m_stream->peek()) != 0) {
        char ch = m_stream->next();
        if (ch == '\n') {
            m_line++;
        }
    }

    Token token{};
    if (!m_stream->has_next()) {
        token.kind = TokenKind::Eof;
        return token;
    }

    char ch = m_stream->next();
    switch (ch) {
    case '+':
        token.kind = TokenKind::Add;
        break;
    case '-':
        token.kind = TokenKind::Sub;
        break;
    case '*':
        token.kind = TokenKind::Mul;
        break;
    case '/':
        token.kind = TokenKind::Div;
        break;
    default:
        if (ch >= '0' && ch <= '9') {
            std::string buf;
            buf += ch;
            while ((ch = m_stream->peek()) >= '0' && ch <= '9') {
                buf += m_stream->next();
            }
            token.kind = TokenKind::NumLit;
            token.num = std::stoi(buf);
        } else {
            throw std::runtime_error("Unexpected character");
        }
    }
    return token;
}
