#pragma once

#include <stdexcept>

namespace abel {

// Stops the program with a message.
// `message` does not have to be valid outside of this invocation.
// This exists to encapsulate the failure handling mechanism, allowing
// to easily switch between, for example, exceptions or panicking.
// May be replaced with a macro for optional/result-based error handling.
[[noreturn]] inline void fail(const char *message) {
    throw std::runtime_error(message);
}

}