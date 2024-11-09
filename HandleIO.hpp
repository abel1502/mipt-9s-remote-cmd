#pragma once

#include <Windows.h>

#include "Handle.hpp"

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

    // Returns true if the last read operation has reached end of stream
    constexpr bool is_eof() const noexcept { return eof; }

    // Reads some data into the buffer. Returns the number of bytes read. Sets eof if the end of the stream is reached.
    size_t read_into(std::span<unsigned char> data);

    // Reads data into the buffer until it is full. Throws and sets eof if end of stream is reached prematurely.
    void read_full_into(std::span<unsigned char> data);

    // Writes the contents. Writes are always complete in a successful invocation.
    void write_from(std::span<const unsigned char> data);

    // TODO: Overlapped (async) interface
};

}  // namespace abel
