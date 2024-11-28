#pragma once

#include "Error.hpp"
#include "IOBase.hpp"

#include <Windows.h>
#include <utility>
#include <vector>
#include <span>
#include <cstdint>
#include <optional>
#include <concepts>
#include <memory>


namespace abel {

class OwningHandle;

template <typename T>
class AIO;

// Handle is a non-owning wrapper around a WinAPI HANDLE
class Handle : public IOBase {
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

    void close();

#pragma region IO
    // Reads some data into the buffer. Returns the number of bytes read and eof status.
    eof<size_t> read_into(std::span<unsigned char> data);

    // Writes the contents. Returns the number of bytes written and eof status. All bytes must be written after a successful invocation.
    eof<size_t> write_from(std::span<const unsigned char> data);

    // Note: asynchronous IO requires the handle to have been opened with FILE_FLAG_OVERLAPPED

    // Cancels all pending async operations on this handle
    void cancel_async();

    // Same as read_into, but returns an awaitable. Note: the buf must not be located in a coroutine stack.
    AIO<eof<size_t>> read_async_into(std::span<unsigned char> data);

    // Same as write_from, but returns an awaitable. Note: the buf must not be located in a coroutine stack.
    AIO<eof<size_t>> write_async_from(std::span<const unsigned char> data);
#pragma endregion IO

#pragma region Synchronization
    static OwningHandle create_event(bool manualReset = false, bool initialState = false, bool inheritHandle = false);

    // TODO: CRITICAL_SECTION appears to be a lighter-weight single-process alternative
    static OwningHandle create_mutex(bool initialOwner = false, bool inheritHandle = false);

    // Sets an event
    void signal();

    // Resets an event
    void reset();

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
#pragma endregion Synchronization

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
        std::swap(value, other.value);
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
