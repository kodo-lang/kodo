#include <Type.hh>

bool operator==(const Type &lhs, const Type &rhs) {
    if (lhs.kind != rhs.kind) {
        return false;
    }
    switch (lhs.kind) {
    case TypeKind::Invalid:
        return true;
    case TypeKind::Int:
        return lhs.bit_width == rhs.bit_width;
    }
}
