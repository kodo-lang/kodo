#pragma once

#include <cstdint>
#include <functional>
#include <utility>

struct PairHash {
    template <typename T, typename U>
    std::size_t operator()(const std::pair<T, U> pair) const {
        return std::hash<T>{}(pair.first) ^ std::hash<U>{}(pair.second);
    }
};
