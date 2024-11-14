#pragma once

#include <Windows.h>
#include <utility>
#include <vector>
#include <span>
#include <cstdint>
#include <optional>
#include <concepts>

#include "Error.hpp"
#include "Owning.hpp"


namespace abel {

class HandleIO;

class Handle {
protected:
    HANDLE value;

public:
    constexpr Handle() noexcept : value(NULL) {}

    constexpr Handle(nullptr_t) noexcept : Handle() {}

    constexpr Handle(HANDLE value) noexcept : value(value) {}

    Handle(const Handle &other) = delete;

    Handle &operator=(const Handle &other) = delete;

    constexpr Handle(Handle &&other) noexcept : value(other.value) {
        other.value = NULL;
    }

    constexpr Handle &&operator=(Handle &&other) noexcept {
        value = other.value;
        other.value = NULL;
        return std::move(*this);
    }

    ~Handle() noexcept {
        if (value) {
            CloseHandle(value);
            value = NULL;
        }
    }

    constexpr operator bool() const noexcept {
        return value != NULL;
    }

    template <typename Self>
    constexpr Self &&validate(this Self &&self) {
        if (self.value == NULL || self.value == INVALID_HANDLE_VALUE) {
            fail("Handle is invalid");
        }
        return std::forward<Self>(self);
    }

    constexpr HANDLE raw() const noexcept {
        return value;
    }

    template <typename Self>
    constexpr decltype(auto) raw_ptr(this Self &&self) noexcept {
        return &self.value;
    }

    Handle clone() const;

#pragma region IO
    HandleIO io() const;

    Owning<HandleIO, Handle> owning_io(this Handle self);
#pragma endregion IO

#pragma region Sync
    static Handle create_event(bool manualReset = false, bool initialState = false, bool inheritHandle = false);

    // TODO: CRITICAL_SECTION appears to be a lighter-weight single-process alternative
    static Handle create_mutex(bool initialOwner = false, bool inheritHandle = false);

    // TODO: signal()

    // Tells if the handle is signaled without waiting
    bool is_signaled() const;

    // Blocks until the handle is signaled
    void wait() const;

    // Returns true if the wait succeeded, false on timeout
    bool wait_timeout(DWORD miliseconds) const;

    // Combines the functionality of wait_timeout, wait (timeout=INFINITE) and is_set (timeout=0)
    // for several handles at once. Returns -1U on timeout.
    template <std::same_as<Handle> ...T>
    static size_t wait_multiple(const T &...handles, bool all = false, DWORD miliseconds = INFINITE) {
        return wait_multiple({&handles...}, all, miliseconds);
    }

    // Same as the template version, but takes a span instead. Has to take pointers instead of references
    static size_t wait_multiple(std::span<const Handle *> handles, bool all = false, DWORD miliseconds = INFINITE);
#pragma endregion Sync

#pragma region Thread
    void suspend_thread() const;

    void resume_thread() const;
#pragma endregion Thread
};

}  // namespace abel
