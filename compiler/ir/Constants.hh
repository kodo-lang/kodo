#pragma once

#include <ir/Constant.hh>

#include <cstdint>
#include <string>
#include <utility>
#include <vector>

namespace ir {

class StructType;

// TODO: Hide constructors?
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
    static ConstantNull *get();

    ConstantNull() noexcept : Constant(KIND, nullptr) {}

    Constant *clone(const Type *type) const override;
};

class ConstantString : public Constant {
    const std::string m_value;

public:
    static constexpr auto KIND = ConstantKind::String;
    static ConstantString *get(std::string value);

    ConstantString(const Type *type, std::string value) : Constant(KIND, type), m_value(std::move(value)) {}

    Constant *clone(const Type *type) const override;

    const std::string &value() const { return m_value; }
};

class ConstantStruct : public Constant {
    const std::vector<Constant *> m_elems;

public:
    static constexpr auto KIND = ConstantKind::Struct;
    static ConstantStruct *get(const StructType *type, std::vector<Constant *> &&elems);

    ConstantStruct(const StructType *type, std::vector<Constant *> &&elems)
        : Constant(KIND, PointerType::get(type, false)), m_elems(std::move(elems)) {}

    Constant *clone(const Type *type) const override;

    const std::vector<Constant *> &elems() const { return m_elems; }
    const StructType *struct_type() const { return type()->as<PointerType>()->pointee_type()->as<StructType>(); }
};

} // namespace ir
