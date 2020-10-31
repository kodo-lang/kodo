#pragma once

#include <cstdint>
#include <string>
#include <variant>

enum class TokenKind {
    Add,
    Ampersand,
    Arrow,
    Cast,
    Colon,
    Comma,
    Div,
    Dot,
    Eof,
    Eq,
    Extern,
    Fn,
    GreaterThan,
    Identifier,
    If,
    LBrace,
    LessThan,
    Let,
    LParen,
    Mul,
    NumLit,
    Return,
    RBrace,
    RParen,
    Semi,
    StringLit,
    Struct,
    Sub,
    Type,
    Var,
};

struct Token {
    TokenKind kind;
    std::variant<std::uint64_t, std::string> data;
};

std::string tok_str(TokenKind kind);
std::string tok_str(const Token &token);
