#include <Lexer.hh>

#include <CharStream.hh>

#include <cctype>
#include <stdexcept>
#include <string>

Token Lexer::next_token() {
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
    case ';':
        token.kind = TokenKind::Semi;
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
        } else if (std::isalpha(ch) != 0) {
            std::string buf;
            buf += ch;
            while (std::isalpha(ch = m_stream->peek()) != 0) {
                buf += m_stream->next();
            }
            if (buf == "return") {
                token.kind = TokenKind::Return;
            } else {
                throw std::runtime_error("Unexpected identifier");
            }
        } else {
            throw std::runtime_error("Unexpected character");
        }
    }
    return token;
}

Token Lexer::next() {
    if (m_peek_ready) {
        m_peek_ready = false;
        return m_peek_token;
    }
    return next_token();
}

Token Lexer::peek() {
    if (!m_peek_ready) {
        m_peek_ready = true;
        m_peek_token = next_token();
    }
    return m_peek_token;
}
