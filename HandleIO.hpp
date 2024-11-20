#pragma once

#include <Windows.h>
#include <vector>
#include <memory>

#include "Handle.hpp"

namespace abel {

class HandleIO {
protected:
    Handle source;
    bool eof = false;

public:
    HandleIO(Handle source) :
        source(source) {
    }

    constexpr HandleIO(const HandleIO &other) noexcept = default;
    constexpr HandleIO &operator=(const HandleIO &other) noexcept = default;
    constexpr HandleIO(HandleIO &&other) noexcept = default;
    constexpr HandleIO &operator=(HandleIO &&other) noexcept = default;

    // Synchronous API

    // Returns true if the last read operation has reached end of stream
    constexpr bool is_eof() const noexcept {
        return eof;
    }

    // Reads some data into the buffer. Returns the number of bytes read. Sets eof if the end of the stream is reached.
    size_t read_into(std::span<unsigned char> data);

    // Reads data into the buffer until it is full. Throws and sets eof if end of stream is reached prematurely.
    void read_full_into(std::span<unsigned char> data);

    // Invokes either read_into or read_full_into and returns the result as a vector
    std::vector<unsigned char> read(size_t size, bool exact = false);

    // Writes the contents. Writes are always complete in a successful invocation.
    void write_from(std::span<const unsigned char> data);

    // Asynchronous API (FILE_FLAG_OVERLAPPED)

    // Cancels all pending async operations on this handle
    void cancel_async();

    // TODO: Think really hard about the proper design for a winapi future
    // TODO: C++20 coroutine-friendlyness:
    //        - await_ready tests GetOverlappedResult (no waiting)
    //        - await_suspend passes this (including the event handle) and the coroutine_handle to the executor (?)
    //        - await_resume calls GetOverlappedResult (no waiting) again, and assumes the result is present
    //        - the executor stores a vector of [type-erased awaitable (?) + coroutine_handle],
    //          and a single step of the executor waits on all events and when one is signaled, removes it
    //          from the list and resumes the associated coroutine. When it is suspended again, it is either
    //          added with a new event, or ...?
    // Returned by async read-write APIs
    class Future {
    protected:
        Handle source;
        std::unique_ptr<OVERLAPPED> overlapped = std::make_unique<OVERLAPPED>();
        bool eof = false;

        Future(Handle source, OwningHandle done) :
            source{source},
            done{std::move(done)} {

            overlapped->hEvent = this->done.raw();
        }

        friend HandleIO;

    public:
        // The event signaling the completion of the operation.
        OwningHandle done;

        // Returns the number of bytes read/written. If a timeout is provided (supports INFINITE), blocks until the operation completes
        // Without a timeout, fails if the operation is incomplete yet. Also sets eof if the end of the stream is reached
        size_t get_result(DWORD miliseconds = 0);

        // Returns true if the operation has reached end of stream. Only effective after the future has been awaited and get_result has been called
        constexpr bool is_eof() const noexcept {
            return eof;
        }

        // TODO: More API
    };

    // Same as read_into, but returns an event signaling the completion of the operation
    [[nodiscard]] Future read_async(std::span<unsigned char> data, OwningHandle doneEvent = Handle::create_event());

    // Same as write_from, but returns an event signaling the completion of the operation
    [[nodiscard]] Future write_async(std::span<const unsigned char> data, OwningHandle doneEvent = Handle::create_event());
};

}  // namespace abel
