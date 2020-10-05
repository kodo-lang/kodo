#include <support/ArgsParser.hh>

#include <support/Assert.hh>

#include <algorithm>
#include <stdexcept>
#include <string_view>

namespace args {

void Parser::parse(int argc, char **argv) {
    auto args = m_args.begin();
    for (int i = 1; i < argc; i++) {
        std::string arg(argv[i]);
        if (!arg.starts_with('-')) {
            // TODO: Merge these.
            **args = std::move(arg);
            args++;
            continue;
        }
        arg = arg.substr(2);
        auto it = std::find_if(m_options.begin(), m_options.end(), [&arg](const Option &option) {
            return arg.starts_with(option.name());
        });
        if (it == m_options.end()) {
            throw std::runtime_error("Unknown option " + arg);
        }
        auto &option = *it;
        auto *value = option.value();
        ASSERT(value != nullptr);
        value->set_present(true);
        arg = arg.substr(option.name().length());
        if (!arg.empty() && arg[0] == '=') {
            value->set_value_passed(true);
            arg = arg.substr(1);
            if (arg == "false" || arg == "0") {
                value->set_value(false);
            } else if (arg == "true" || arg == "1") {
                value->set_value(true);
            } else {
                auto *string_value = reinterpret_cast<Value<std::string> *>(value);
                string_value->set_value(std::move(arg));
            }
        }
    }

    if (args != m_args.end()) {
        throw std::runtime_error("Invalid number of args passed");
    }
}

} // namespace args
