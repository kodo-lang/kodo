#pragma once

#include <cstdint>
#include <string>
#include <variant>

enum class TokenKind {
    Add,
    Ampersand,
    Arrow,
    Asm,
    Cast,
    Clobber,
    Colon,
    Comma,
    Div,
    Dot,
    DoubleColon,
    Eof,
    Eq,
    Extern,
    Fn,
    GreaterThan,
    Identifier,
    If,
    Import,
    In,
    LBrace,
    LessThan,
    Let,
    LParen,
    Mul,
    Mut,
    NumLit,
    Output,
    Return,
    RBrace,
    RParen,
    Semi,
    StringLit,
    Struct,
    Sub,
    This,
    Type,
    Var,
};

struct Token {
    TokenKind kind;
    std::variant<std::uint64_t, std::string> data;
};

std::string tok_str(TokenKind kind);
std::string tok_str(const Token &token);
