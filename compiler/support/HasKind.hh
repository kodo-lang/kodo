#pragma once

template <typename T, typename Kind>
concept HasKind = requires(T *t) {
    static_cast<Kind>(T::KIND);
};
