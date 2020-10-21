#include <support/Error.hh>

bool g_error = false;

void abort_if_error() {
    if (g_error) {
        print_note("Aborting due to previous errors");
        std::exit(1);
    }
}
