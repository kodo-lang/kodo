#pragma once

struct PassResult {
    PassResult() = default;
    PassResult(const PassResult &) = delete;
    PassResult(PassResult &&) = delete;
    virtual ~PassResult() = default;

    PassResult &operator=(const PassResult &) = delete;
    PassResult &operator=(PassResult &&) = delete;
};
