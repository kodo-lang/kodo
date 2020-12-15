#pragma once

#include <ir/Constant.hh>

#include <cstdint>
#include <string>
#include <utility>
#include <vector>

namespace ir {

class ArrayType;
class Program;

// TODO: Hide constructors?

class ConstantArray : public Constant {
    // TODO: Make this `Constant *` instead of `Value *` when functions are constants.
    const std::vector<Value *> m_elems;

public:
    static constexpr auto KIND = ConstantKind::Array;
    static ConstantArray *get(const ArrayType *type, std::vector<Value *> &&elems);
    static ConstantArray *get(std::vector<Value *> &&elems);

    ConstantArray(const Type *type, std::vector<Value *> &&elems)
        : Constant(KIND, type), m_elems(std::move(elems)) {}

    Constant *clone(const Type *type) const override;

    const std::vector<Value *> &elems() const { return m_elems; }
};

class ConstantInt : public Constant {
    const std::size_t m_value;

public:
    static constexpr auto KIND = ConstantKind::Int;
    static ConstantInt *get(const Type *type, std::size_t value);

    ConstantInt(const Type *type, std::size_t value) : Constant(KIND, type), m_value(value) {}

    Constant *clone(const Type *type) const override;

    std::size_t value() const { return m_value; }
};

struct ConstantNull : public Constant {
    static constexpr auto KIND = ConstantKind::Null;
    static ConstantNull *get(const Type *type);

    explicit ConstantNull(const Type *type) : Constant(KIND, type) {}

    Constant *clone(const Type *type) const override;
};

class ConstantString : public Constant {
    const std::string m_value;

public:
    static constexpr auto KIND = ConstantKind::String;
    static ConstantString *get(const Program *program, std::string value);

    ConstantString(const Type *type, std::string value) : Constant(KIND, type), m_value(std::move(value)) {}

    Constant *clone(const Type *type) const override;

    const std::string &value() const { return m_value; }
};

class Undef : public Constant {
public:
    static constexpr auto KIND = ConstantKind::Undef;
    static Undef *get(const Type *type);

    explicit Undef(const Type *type) : Constant(KIND, type) {}

    Constant *clone(const Type *type) const override;
};

} // namespace ir
