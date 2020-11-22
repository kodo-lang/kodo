#pragma once

#include <ir/Type.hh>

#include <string>
#include <vector>

namespace ir {

struct InvalidType : public Type {
    static constexpr auto KIND = TypeKind::Invalid;

    explicit InvalidType(const TypeCache *cache) : Type(cache, KIND) {}

    std::string to_string() const override;
};

struct BoolType : public Type {
    static constexpr auto KIND = TypeKind::Bool;

    explicit BoolType(const TypeCache *cache) : Type(cache, KIND) {}

    std::string to_string() const override;
};

class IntType : public Type {
    const int m_bit_width;
    const bool m_is_signed;

public:
    static constexpr auto KIND = TypeKind::Int;

    IntType(const TypeCache *cache, int bit_width, bool is_signed)
        : Type(cache, KIND), m_bit_width(bit_width), m_is_signed(is_signed) {}

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

    PointerType(const TypeCache *cache, const Type *pointee_type, bool is_mutable)
        : Type(cache, KIND), m_pointee_type(pointee_type), m_is_mutable(is_mutable) {}

    std::string to_string() const override;

    const Type *pointee_type() const { return m_pointee_type; }
    bool is_mutable() const { return m_is_mutable; }
};

class StructField {
    const std::string m_name;
    const Type *const m_type;

public:
    StructField(std::string name, const Type *type) : m_name(std::move(name)), m_type(type) {}

    bool operator==(const StructField &rhs) const {
        return m_type == rhs.m_type && m_name == rhs.m_name;
    }

    const std::string &name() const { return m_name; }
    const Type *type() const { return m_type; }
};

class StructType : public Type {
    std::vector<StructField> m_fields;

public:
    static constexpr auto KIND = TypeKind::Struct;

    StructType(const TypeCache *cache, std::vector<StructField> &&fields)
        : Type(cache, KIND), m_fields(std::move(fields)) {}

    int size_in_bytes() const override;
    std::string to_string() const override;

    const std::vector<StructField> &fields() const { return m_fields; }
};

struct VoidType : public Type {
    static constexpr auto KIND = TypeKind::Void;

    explicit VoidType(const TypeCache *cache) : Type(cache, KIND) {}

    std::string to_string() const override;
};

} // namespace ir
