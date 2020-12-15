#pragma once

#include <fmt/color.h>
#include <fmt/core.h>

// TODO: Global :(
extern bool g_error;

template <typename T>
concept HasLine = requires(const T *t) {
    static_cast<int>(t->line());
};

template <typename FmtStr, typename... Args>
void print_error(const FmtStr &fmt, const Args &... args) {
    g_error = true;
    auto formatted = fmt::format(fmt, args...);
    auto error = fmt::format(fmt::fg(fmt::color::orange_red), "error:");
    fmt::print("{} {}\n", error, formatted);
}

template <typename T, typename FmtStr, typename... Args>
void print_error(T *obj, const FmtStr &fmt, const Args &... args) requires HasLine<T> {
    auto formatted = fmt::format(fmt, args...);
    print_error("{} on line {}", formatted, obj->line());
}

template <typename T, typename FmtStr, typename... Args>
void print_error(const T *obj, const FmtStr &fmt, const Args &... args) requires HasLine<T> {
    auto formatted = fmt::format(fmt, args...);
    print_error("{} on line {}", formatted, obj->line());
}

template <typename FmtStr, typename... Args>
void print_note(const FmtStr &fmt, const Args &... args) {
    auto formatted = fmt::format(fmt, args...);
    auto note = fmt::format(fmt::fg(fmt::color::slate_blue), " note:");
    fmt::print("{} {}\n", note, formatted);
}

template <typename FmtStr, typename... Args>
[[noreturn]] void print_error_and_abort(const FmtStr &fmt, const Args &... args) {
    print_error(fmt, args...);
    print_note("Aborting due to previous error");
    std::exit(1);
}

void abort_if_error();
