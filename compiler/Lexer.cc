#include <Lexer.hh>

#include <CharStream.hh>

#include <fmt/color.h>
#include <fmt/core.h>

#include <cctype>
#include <string>

namespace {

template <typename FmtString, typename... Args>
void error(const FmtString &fmt, const Args &... args) {
    auto formatted = fmt::format(fmt, args...);
    fmt::print(fmt::fg(fmt::color::orange_red), "lexer: {}\n", formatted);
    abort();
}

} // namespace

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

    auto consume_if = [&](char ch) {
        if (m_stream->peek() == ch) {
            m_stream->next();
            return true;
        }
        return false;
    };

    char ch = m_stream->next();
    switch (ch) {
    case '+':
        token.kind = TokenKind::Add;
        break;
    case '-':
        token.kind = consume_if('>') ? TokenKind::Arrow : TokenKind::Sub;
        break;
    case '*':
        token.kind = TokenKind::Mul;
        break;
    case '/':
        token.kind = TokenKind::Div;
        break;
    case '{':
        token.kind = TokenKind::LBrace;
        break;
    case '}':
        token.kind = TokenKind::RBrace;
        break;
    case '(':
        token.kind = TokenKind::LParen;
        break;
    case ')':
        token.kind = TokenKind::RParen;
        break;
    case '=':
        token.kind = TokenKind::Eq;
        break;
    case '&':
        token.kind = TokenKind::Ampersand;
        break;
    case ':':
        token.kind = TokenKind::Colon;
        break;
    case ',':
        token.kind = TokenKind::Comma;
        break;
    case ';':
        token.kind = TokenKind::Semi;
        break;
    default:
        if (std::isdigit(ch) != 0) {
            std::string buf;
            buf += ch;
            while (std::isdigit(m_stream->peek()) != 0) {
                buf += m_stream->next();
            }
            token.kind = TokenKind::NumLit;
            token.num = std::stoi(buf);
        } else if (std::isalpha(ch) != 0) {
            std::string buf;
            buf += ch;
            while (std::isalpha(ch = m_stream->peek()) != 0 || std::isdigit(ch) != 0) {
                buf += m_stream->next();
            }
            if (buf == "fn") {
                token.kind = TokenKind::Fn;
            } else if (buf == "return") {
                token.kind = TokenKind::Return;
            } else if (buf == "var") {
                token.kind = TokenKind::Var;
            } else {
                auto *ptr = new char[buf.size() + 1];
                std::memcpy(ptr, buf.c_str(), buf.size());
                ptr[buf.size()] = '\0';
                token.kind = TokenKind::Identifier;
                token.text = ptr;
            }
        } else {
            error("unexpected '{}' on line {}", ch, m_line);
        }
    }
    return token;
}

bool Lexer::has_next() {
    return m_stream->has_next();
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
