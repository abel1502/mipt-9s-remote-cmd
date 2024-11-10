#pragma once

#include <Windows.h>
#include <vector>

#include "Handle.hpp"
#include "Sync.hpp"

namespace abel {

class HandleIO {
protected:
    HANDLE source;
    bool eof = false;

public:
    HandleIO(HANDLE source) : source(source) {}

    constexpr HandleIO(const HandleIO &other) noexcept = default;
    constexpr HandleIO &operator=(const HandleIO &other) noexcept = default;
    constexpr HandleIO(HandleIO &&other) noexcept = default;
    constexpr HandleIO &operator=(HandleIO &&other) noexcept = default;

    // Synchronous API

    // Returns true if the last read operation has reached end of stream
    constexpr bool is_eof() const noexcept { return eof; }

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

    // TODO: OVERLAPPED must be preserved for the duration of the asynchronous operation, gotta return it alongside the event
    // TODO: Also gotta use GetOverlappedResult to extract the read/written count and the status.
    // TODO: Gotta mark the return nodiscard

    // Same as read_into, but returns an event signaling the completion of the operation. If read is not null, returns the number of bytes read
    Sync read_async(std::span<unsigned char> data, DWORD *read = nullptr);

    // Same as write_from, but returns an event signaling the completion of the operation
    Sync write_async(std::span<const unsigned char> data, DWORD *written = nullptr);
};

}  // namespace abel
