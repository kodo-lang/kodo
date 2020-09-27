#pragma once

#include <cstdint>
#include <string>
#include <variant>

enum class TokenKind {
    Add,
    Ampersand,
    Arrow,
    Colon,
    Comma,
    Div,
    Eof,
    Eq,
    Extern,
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
    std::variant<std::uint64_t, std::string> data;
};

std::string tok_str(TokenKind kind);
std::string tok_str(const Token &token);
