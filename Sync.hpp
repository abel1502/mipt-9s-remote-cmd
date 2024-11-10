#pragma once

#include <Windows.h>

#include "Handle.hpp"

namespace abel {

// A common class for various synchronization objects (events, mutexes, semaphores, ...)
class Sync {
protected:
    Sync() {}

public:
    Handle handle{};

    constexpr Sync(Sync &&other) noexcept = default;
    constexpr Sync &operator=(Sync &&other) noexcept = default;

    static Sync create_event(bool manualReset = false, bool initialState = false, bool inheritHandle = false);

    // TODO: CRITICAL_SECTION appears to be a lighter-weight single-process alternative
    static Sync create_mutex(bool initialOwner = false, bool inheritHandle = false);

};

}  // namespace abel
