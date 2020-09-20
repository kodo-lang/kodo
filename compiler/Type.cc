#include <Type.hh>

// TODO: Just compare types by pointer when we have a type cache.
bool operator==(const Type &lhs, const Type &rhs) {
    if (lhs.kind() != rhs.kind()) {
        return false;
    }
    switch (lhs.kind()) {
    case TypeKind::Invalid:
        return true;
    case TypeKind::Int:
        return lhs.as<IntType>()->bit_width() == rhs.as<IntType>()->bit_width();
    }
}
