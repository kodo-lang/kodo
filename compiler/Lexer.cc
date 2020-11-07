#include <Lexer.hh>

#include <CharStream.hh>
#include <support/Error.hh>

#include <cctype>
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
        if (consume_if('/')) {
            while (m_stream->peek() != '\n') {
                m_stream->next();
            }
            return next_token();
        }
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
    case '<':
        token.kind = TokenKind::LessThan;
        break;
    case '>':
        token.kind = TokenKind::GreaterThan;
        break;
    case '&':
        token.kind = TokenKind::Ampersand;
        break;
    case ':':
        token.kind = consume_if(':') ? TokenKind::DoubleColon : TokenKind::Colon;
        break;
    case ',':
        token.kind = TokenKind::Comma;
        break;
    case '.':
        token.kind = TokenKind::Dot;
        break;
    case ';':
        token.kind = TokenKind::Semi;
        break;
    case '"': {
        std::string buf;
        while (m_stream->peek() != '"') {
            buf += m_stream->next();
        }
        m_stream->next();
        token.kind = TokenKind::StringLit;
        token.data = std::move(buf);
        break;
    }
    default:
        if (std::isdigit(ch) != 0) {
            std::string buf;
            buf += ch;
            while (std::isdigit(m_stream->peek()) != 0) {
                buf += m_stream->next();
            }
            token.kind = TokenKind::NumLit;
            token.data = static_cast<std::uint64_t>(std::stol(buf));
        } else if (std::isalpha(ch) != 0 || ch == '_') {
            std::string buf;
            buf += ch;
            while (std::isalpha(ch = m_stream->peek()) != 0 || std::isdigit(ch) != 0 || ch == '_') {
                buf += m_stream->next();
            }
            if (buf == "asm") {
                token.kind = TokenKind::Asm;
            } else if (buf == "cast") {
                token.kind = TokenKind::Cast;
            } else if (buf == "clobber") {
                token.kind = TokenKind::Clobber;
            } else if (buf == "extern") {
                token.kind = TokenKind::Extern;
            } else if (buf == "fn") {
                token.kind = TokenKind::Fn;
            } else if (buf == "if") {
                token.kind = TokenKind::If;
            } else if (buf == "import") {
                token.kind = TokenKind::Import;
            } else if (buf == "in") {
                token.kind = TokenKind::In;
            } else if (buf == "let") {
                token.kind = TokenKind::Let;
            } else if (buf == "mut") {
                token.kind = TokenKind::Mut;
            } else if (buf == "output") {
                token.kind = TokenKind::Output;
            } else if (buf == "return") {
                token.kind = TokenKind::Return;
            } else if (buf == "struct") {
                token.kind = TokenKind::Struct;
            } else if (buf == "this") {
                token.kind = TokenKind::This;
            } else if (buf == "type") {
                token.kind = TokenKind::Type;
            } else if (buf == "var") {
                token.kind = TokenKind::Var;
            } else {
                token.kind = TokenKind::Identifier;
                token.data = std::move(buf);
            }
        } else {
            print_error_and_abort("unexpected '{}' on line {}", ch, m_line);
        }
    }
    return token;
}

bool Lexer::has_next() {
    return m_stream->has_next() && peek().kind != TokenKind::Eof;
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
