#pragma once

#include <support/Assert.hh>
#include <support/Castable.hh>

#include <string>
#include <vector>

namespace ir {

enum class TypeKind {
    Invalid,
    Bool,
    Int,
    Pointer,
    Struct,
    Void,
};

class Type : public Castable<Type, TypeKind, false> {
    TypeKind m_kind;

protected:
    explicit Type(TypeKind kind) : m_kind(kind) {}

public:
    Type(const Type &) = delete;
    Type(Type &&) = delete;
    virtual ~Type() = default;

    Type &operator=(const Type &) = delete;
    Type &operator=(Type &&) = delete;

    virtual int size_in_bytes() const;
    virtual std::string to_string() const = 0;

    TypeKind kind() const { return m_kind; }
};

struct InvalidType : public Type {
    static constexpr auto KIND = TypeKind::Invalid;
    static const InvalidType *get();

    InvalidType() noexcept : Type(KIND) {}

    std::string to_string() const override;
};

struct BoolType : public Type {
    static constexpr auto KIND = TypeKind::Bool;
    static const BoolType *get();

    BoolType() noexcept : Type(KIND) {}

    std::string to_string() const override;
};

class IntType : public Type {
    const int m_bit_width;
    const bool m_is_signed;

public:
    static constexpr auto KIND = TypeKind::Int;
    static const IntType *get(int bit_width, bool is_signed);
    static const IntType *get_signed(int bit_width);
    static const IntType *get_unsigned(int bit_width);

    IntType(int bit_width, bool is_signed) : Type(KIND), m_bit_width(bit_width), m_is_signed(is_signed) {}

    int size_in_bytes() const override;
    std::string to_string() const override;

    int bit_width() const { return m_bit_width; }
    bool is_signed() const { return m_is_signed; }
};

class PointerType : public Type {
    const Type *m_pointee_type;
    bool m_is_mutable;

public:
    static constexpr auto KIND = TypeKind::Pointer;
    static const PointerType *get(const Type *pointee_type, bool is_mutable);

    PointerType(const Type *pointee_type, bool is_mutable)
        : Type(KIND), m_pointee_type(pointee_type), m_is_mutable(is_mutable) {}

    std::string to_string() const override;

    const Type *pointee_type() const { return m_pointee_type; }
    bool is_mutable() const { return m_is_mutable; }
};

class StructType : public Type {
    std::vector<const Type *> m_fields;

public:
    static constexpr auto KIND = TypeKind::Struct;
    static const StructType *get(std::vector<const Type *> &&fields);

    explicit StructType(std::vector<const Type *> &&fields) : Type(KIND), m_fields(std::move(fields)) {}

    int size_in_bytes() const override;
    std::string to_string() const override;

    const std::vector<const Type *> &fields() const { return m_fields; }
};

struct VoidType : public Type {
    static constexpr auto KIND = TypeKind::Void;
    static const VoidType *get();

    std::string to_string() const override;

    VoidType() noexcept : Type(KIND) {}
};

} // namespace ir
