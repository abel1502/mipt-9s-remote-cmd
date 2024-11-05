#include "Handle.hpp"

Handle Handle::clone() const {
    Handle result{};

    bool success = DuplicateHandle(
        GetCurrentProcess(),
        value,
        GetCurrentProcess(),
        &result.value,
        0,
        false,
        DUPLICATE_SAME_ACCESS
    );

    if (!success) {
        throw std::runtime_error("Failed to duplicate handle");
    }

    return result;
}