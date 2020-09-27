#pragma once

enum class TypeKind {
    Invalid,
    Int,
    Pointer,
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

    TypeKind kind() const { return m_kind; }
};

class IntType : public Type {
    const int m_bit_width;

public:
    static constexpr auto kind = TypeKind::Int;
    static const IntType *get(int bit_width);

    explicit IntType(int bit_width) : Type(kind), m_bit_width(bit_width) {}

    int bit_width() const { return m_bit_width; }
};

class PointerType : public Type {
    const Type *m_pointee_type;

public:
    static constexpr auto kind = TypeKind::Pointer;
    static const PointerType *get(const Type *pointee_type);

    explicit PointerType(const Type *pointee_type) : Type(kind), m_pointee_type(pointee_type) {}

    const Type *pointee_type() const { return m_pointee_type; }
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

bool operator==(const Type &lhs, const Type &rhs);
