#pragma once

#include <ir/Prototype.hh>
#include <ir/Type.hh>
#include <support/List.hh>

#include <string>
#include <utility>
#include <vector>

namespace ir {

// TODO: Cleanup const-correctness.

struct InvalidType : public Type {
    static constexpr auto KIND = TypeKind::Invalid;

    explicit InvalidType(const TypeCache *cache) : Type(cache, KIND) {}

    std::string to_string() const override;
};

class AliasType : public Type {
    const Type *const m_aliased;
    const std::string m_name;

public:
    static constexpr auto KIND = TypeKind::Alias;

    AliasType(const TypeCache *cache, const Type *aliased, std::string name)
        : Type(cache, KIND), m_aliased(aliased), m_name(std::move(name)) {}

    bool equals_weak(const Type *other) const override;
    std::string to_string() const override;

    const Type *aliased() const { return m_aliased; }
    const std::string &name() const { return m_name; }
};

class ArrayType : public Type {
    const Type *const m_element_type;
    const std::size_t m_length;

public:
    static constexpr auto KIND = TypeKind::Array;

    ArrayType(const TypeCache *cache, const Type *element_type, std::size_t length)
        : Type(cache, KIND), m_element_type(element_type), m_length(length) {}

    std::string to_string() const override;

    const Type *element_type() const { return m_element_type; }
    std::size_t length() const { return m_length; }
};

struct BoolType : public Type {
    static constexpr auto KIND = TypeKind::Bool;

    explicit BoolType(const TypeCache *cache) : Type(cache, KIND) {}

    std::string to_string() const override;
};

class FunctionType : public Type {
    const Type *const m_return_type;
    const std::vector<const Type *> m_params;

public:
    static constexpr auto KIND = TypeKind::Function;

    FunctionType(const TypeCache *cache, const Type *return_type, std::vector<const Type *> &&params)
        : Type(cache, KIND), m_return_type(return_type), m_params(std::move(params)) {}

    std::string to_string() const override;

    const Type *return_type() const { return m_return_type; }
    const std::vector<const Type *> &params() const { return m_params; }
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

    bool equals_weak(const Type *other) const override;
    std::string to_string() const override;

    const Type *pointee_type() const { return m_pointee_type; }
    bool is_mutable() const { return m_is_mutable; }
};

class StructField {
    const std::string m_name;
    const Type *const m_type;

public:
    StructField(std::string name, const Type *type) : m_name(std::move(name)), m_type(type) {}

    bool operator==(const StructField &rhs) const { return m_name == rhs.m_name && m_type == rhs.m_type; }

    const std::string &name() const { return m_name; }
    const Type *type() const { return m_type; }
};

class StructType : public Type {
    std::vector<StructField> m_fields;
    std::vector<const Type *> m_implementing;
    mutable List<Prototype> m_prototypes;

public:
    static constexpr auto KIND = TypeKind::Struct;

    explicit StructType(const TypeCache *cache) : Type(cache, KIND) {}

    void add_field(const std::string &name, const Type *type) { m_fields.emplace_back(name, type); };
    void add_implementing(const Type *type) { m_implementing.push_back(type); }
    void add_prototype(Prototype *prototype) const { m_prototypes.insert(m_prototypes.end(), prototype); }
    int size_in_bytes() const override;
    std::string to_string() const override;

    const std::vector<StructField> &fields() const { return m_fields; }
    const std::vector<const Type *> &implementing() const { return m_implementing; }
    const List<Prototype> &prototypes() const { return m_prototypes; }
};

class TraitType : public Type {
    List<Prototype> m_prototypes;

public:
    static constexpr auto KIND = TypeKind::Trait;

    explicit TraitType(const TypeCache *cache) : Type(cache, KIND) {}

    void add_prototype(Prototype *prototype) { m_prototypes.insert(m_prototypes.end(), prototype); }
    std::string to_string() const override;

    const List<Prototype> &prototypes() const { return m_prototypes; }
};

struct VoidType : public Type {
    static constexpr auto KIND = TypeKind::Void;

    explicit VoidType(const TypeCache *cache) : Type(cache, KIND) {}

    std::string to_string() const override;
};

template <typename Aliased>
const Aliased *Type::base_as(const Type *type) {
    return base(type)->as_or_null<Aliased>();
}

template <typename Aliased>
std::pair<std::string, const Aliased *> Type::expand_alias(const Type *type) {
    return std::make_pair(Type::name(type), base_as<Aliased>(type));
}

} // namespace ir
