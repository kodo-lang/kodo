#include <support/Assert.hh>

#include <fmt/color.h>
#include <fmt/core.h>

[[noreturn]] void assertion_failed(const char *file, unsigned int line, const char *expr) {
    auto error = fmt::format(fmt::fg(fmt::color::orange_red), "error:");
    fmt::print("{} Assertion '{}' failed at {}:{}\n", error, expr, file, line);
    fmt::print(fmt::fg(fmt::color::orange_red), " note: This is a compiler bug!\n");
    abort();
}
