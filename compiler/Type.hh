#pragma once

#include <string>

enum class TypeKind {
    Invalid,
    Bool,
    Int,
    Pointer,
    Void,
};

class Type {
    TypeKind m_kind;

public:
    explicit Type(TypeKind kind) : m_kind(kind) {}
    Type(const Type &) = delete;
    Type(Type &&) = delete;
    virtual ~Type() = default;

    Type &operator=(const Type &) = delete;
    Type &operator=(Type &&) = delete;

    template <typename T>
    const T *as() const;

    template <typename T>
    bool is() const;

    virtual std::string to_string() const = 0;

    TypeKind kind() const { return m_kind; }
};

// TODO: Make cast system more like IR.
struct InvalidType : public Type {
    static constexpr auto kind = TypeKind::Invalid;
    static const InvalidType *get();

    InvalidType() noexcept : Type(kind) {}

    std::string to_string() const override;
};

struct BoolType : public Type {
    static constexpr auto kind = TypeKind::Bool;
    static const BoolType *get();

    std::string to_string() const override;

    BoolType() noexcept : Type(kind) {}
};

class IntType : public Type {
    const int m_bit_width;

public:
    static constexpr auto kind = TypeKind::Int;
    static const IntType *get(int bit_width);

    explicit IntType(int bit_width) : Type(kind), m_bit_width(bit_width) {}

    std::string to_string() const override;

    int bit_width() const { return m_bit_width; }
};

class PointerType : public Type {
    const Type *m_pointee_type;

public:
    static constexpr auto kind = TypeKind::Pointer;
    static const PointerType *get(const Type *pointee_type);

    explicit PointerType(const Type *pointee_type) : Type(kind), m_pointee_type(pointee_type) {}

    std::string to_string() const override;

    const Type *pointee_type() const { return m_pointee_type; }
};

struct VoidType : public Type {
    static constexpr auto kind = TypeKind::Void;
    static const VoidType *get();

    std::string to_string() const override;

    VoidType() noexcept : Type(kind) {}
};

template <typename T>
const T *Type::as() const {
    if (!is<T>()) {
        return nullptr;
    }
    return static_cast<const T *>(this);
}

template <typename T>
bool Type::is() const {
    return m_kind == T::kind;
}
