#pragma once

#include <support/Castable.hh>

#include <string>

namespace ir {

class TypeCache;

enum class TypeKind {
    Invalid,
    Alias,
    Bool,
    Function,
    Int,
    Pointer,
    Struct,
    Void,
};

class Type : public Castable<Type, TypeKind, false> {
    const TypeCache *const m_cache;
    const TypeKind m_kind;

protected:
    Type(const TypeCache *cache, TypeKind kind) : m_cache(cache), m_kind(kind) {}

public:
    static const Type *base(const Type *type);
    template <typename Aliased>
    static const Aliased *base_as(const Type *type);
    template <typename Aliased>
    static std::pair<std::string, const Aliased *> expand_alias(const Type *type);
    static std::string name(const Type *type);

    Type(const Type &) = delete;
    Type(Type &&) = delete;
    virtual ~Type() = default;

    Type &operator=(const Type &) = delete;
    Type &operator=(Type &&) = delete;

    virtual bool equals_weak(const Type *other) const;
    virtual int size_in_bytes() const;
    virtual std::string to_string() const = 0;

    const TypeCache *cache() const { return m_cache; }
    TypeKind kind() const { return m_kind; }
};

} // namespace ir
