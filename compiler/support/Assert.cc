#include <support/Assert.hh>

#include <support/Error.hh>

[[noreturn]] void assertion_failed(const char *file, unsigned int line, const char *expr) {
    print_error("Assertion '{}' failed at {}:{}", expr, file, line);
    print_note("This is a compiler bug!");
    std::exit(1);
}
