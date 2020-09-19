#pragma once

#include <istream>

class CharStream {
    std::istream *const m_stream;

public:
    explicit CharStream(std::istream *stream) : m_stream(stream) {}

    bool has_next();
    char peek();
    char next();
};
