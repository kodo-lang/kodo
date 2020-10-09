#pragma once

#include <fmt/color.h>
#include <fmt/core.h>

#include <string>

template <typename T, typename FmtStr, typename... Args>
std::string format_error(const T *obj, const FmtStr &fmt, const Args &... args) {
    auto formatted = fmt::format(fmt, args...);
    auto error = fmt::format(fmt::fg(fmt::color::orange_red), "error:");
    return fmt::format("{} {} on line {}\n", error, formatted, obj->line());
}
