#pragma once

#include <cstdint>
#include <string>

enum class TokenKind {
    Add,
    Div,
    Eof,
    Mul,
    NumLit,
    Sub,
};

struct Token {
    TokenKind kind;
    union {
        std::uint64_t num;
    };
};

std::string tok_str(const Token &token);
