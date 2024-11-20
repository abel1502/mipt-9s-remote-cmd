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

class OwningHandle;

// Handle is a non-owning wrapper around a WinAPI HANDLE
class Handle {
protected:
    HANDLE value;

public:
    constexpr Handle() noexcept :
        value(NULL) {
    }

    constexpr Handle(nullptr_t) noexcept :
        Handle() {
    }

    constexpr Handle(HANDLE value) noexcept :
        value(value) {
    }

    constexpr Handle(const Handle &other) noexcept = default;
    constexpr Handle &operator=(const Handle &other) noexcept = default;
    constexpr Handle(Handle &&other) noexcept = default;
    constexpr Handle &operator=(Handle &&other) noexcept = default;

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

    OwningHandle clone() const;

#pragma region IO
    // TODO: incorporate directly?
    HandleIO io() const;
#pragma endregion IO

#pragma region Sync
    static OwningHandle create_event(bool manualReset = false, bool initialState = false, bool inheritHandle = false);

    // TODO: CRITICAL_SECTION appears to be a lighter-weight single-process alternative
    static OwningHandle create_mutex(bool initialOwner = false, bool inheritHandle = false);

    // TODO: signal()

    // Tells if the handle is signaled without waiting
    bool is_signaled() const;

    // Blocks until the handle is signaled
    void wait() const;

    // Returns true if the wait succeeded, false on timeout
    bool wait_timeout(DWORD miliseconds) const;

    // Combines the functionality of wait_timeout, wait (timeout=INFINITE) and is_set (timeout=0)
    // for several handles at once. Returns -1U on timeout
    template <std::same_as<Handle>... T>
    static size_t wait_multiple(T ...handles, bool all = false, DWORD miliseconds = INFINITE) {
        return wait_multiple({handles...}, all, miliseconds);
    }

    // Same as the template version, but takes a span instead
    static size_t wait_multiple(std::span<Handle> handles, bool all = false, DWORD miliseconds = INFINITE);
#pragma endregion Sync

#pragma region Thread
    void suspend_thread() const;

    void resume_thread() const;
#pragma endregion Thread
};

// A handle that closes itself on destruction
class OwningHandle : public Handle {
public:
    constexpr OwningHandle() noexcept :
        Handle() {
    }

    constexpr OwningHandle(nullptr_t) noexcept :
        Handle(nullptr) {
    }

    constexpr OwningHandle(HANDLE value) noexcept :
        Handle(value) {
    }

    OwningHandle(const OwningHandle &other) = delete;

    OwningHandle &operator=(const OwningHandle &other) = delete;

    constexpr OwningHandle(OwningHandle &&other) noexcept :
        Handle(std::move(other)) {
        other.value = NULL;
    }

    constexpr OwningHandle &operator=(OwningHandle &&other) noexcept {
        value = other.value;
        other.value = NULL;
        return *this;
    }

    ~OwningHandle() noexcept {
        if (value) {
            CloseHandle(value);
            value = NULL;
        }
    }
};


}  // namespace abel
