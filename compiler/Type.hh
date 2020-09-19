#pragma once

enum class TypeKind {
    Invalid,
    Int,
};

struct Type {
    TypeKind kind;
    union {
        int bit_width;
    };
};

bool operator==(const Type &lhs, const Type &rhs);
