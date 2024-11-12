#pragma once

#include <Windows.h>

#include "Handle.hpp"

namespace abel {

// A common class for various synchronization objects (events, mutexes, semaphores, ...)
class Sync : public Handle {
public:
    constexpr Sync() : Handle() {}
    constexpr Sync(nullptr_t) : Handle(nullptr) {}
    constexpr Sync(HANDLE handle) : Handle(handle) {}

    constexpr Sync(Sync &&other) noexcept = default;
    constexpr Sync &operator=(Sync &&other) noexcept = default;

    static Sync create_event(bool manualReset = false, bool initialState = false, bool inheritHandle = false);

    // TODO: CRITICAL_SECTION appears to be a lighter-weight single-process alternative
    static Sync create_mutex(bool initialOwner = false, bool inheritHandle = false);

    // Tells if the handle is signaled without waiting
    bool is_set() const;

    // Blocks until the handle is signaled
    void wait() const;

    // Returns true if the wait succeeded, false on timeout
    bool wait_timeout(DWORD miliseconds) const;

};

}  // namespace abel
