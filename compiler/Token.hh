#pragma once

#include <cstdint>
#include <string>

enum class TokenKind {
    Add,
    Arrow,
    Colon,
    Div,
    Eof,
    Eq,
    Fn,
    Identifier,
    LBrace,
    LParen,
    Mul,
    NumLit,
    Return,
    RBrace,
    RParen,
    Semi,
    Sub,
    Var,
};

struct Token {
    TokenKind kind;
    union {
        std::uint64_t num;
        const char *text;
    };
};

std::string tok_str(TokenKind kind);
std::string tok_str(const Token &token);
