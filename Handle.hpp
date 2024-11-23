#pragma once

#include "Error.hpp"

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

    void close();

#pragma region IO
#pragma region Synchronous
    // TODO: Return EOF too

    // Reads some data into the buffer. Returns the number of bytes read. Sets eof if the end of the stream is reached.
    size_t read_into(std::span<unsigned char> data);

    // Reads data into the buffer until it is full. Throws and sets eof if end of stream is reached prematurely.
    void read_full_into(std::span<unsigned char> data);

    // Invokes either read_into or read_full_into and returns the result as a vector
    std::vector<unsigned char> read(size_t size, bool exact = false);

    // Writes the contents. Writes are always complete in a successful invocation.
    void write_from(std::span<const unsigned char> data);
#pragma endregion Synchronous

#pragma region Asynchronous
    // Note: requires the handle to have been opened with FILE_FLAG_OVERLAPPED

    // TODO: Return EOF too

    // Cancels all pending async operations on this handle
    void cancel_async();

    // Same as read_into, but returns an awaitable
    AIO<size_t> read_async(std::span<unsigned char> data);

    // TODO: Maybe not void? IDK if it can fail
    // Same as write_from, but returns an awaitable
    AIO<void> write_async(std::span<const unsigned char> data);
#pragma endregion Asynchronous
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
