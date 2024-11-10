#include "HandleIO.hpp"

#include <stdexcept>
#include <cassert>

namespace abel {

size_t HandleIO::read_into(std::span<unsigned char> data) {
    DWORD read = 0;
    bool success = ReadFile(source, data.data(), data.size(), &read, nullptr);
    if (!success) {
        throw std::runtime_error("Failed to read from handle");
    }

    eof = read == 0;

    return read;
}

void HandleIO::read_full_into(std::span<unsigned char> data) {
    while (data.size() > 0 && !eof) {
        size_t read = read_into(data);
        data = data.subspan(read);
    }

    if (eof) {
        throw std::runtime_error("End of stream reached prematurely");
    }
}

void HandleIO::write_from(std::span<const unsigned char> data) {
    DWORD written = 0;
    bool success = WriteFile(source, data.data(), data.size(), &written, nullptr);
    if (!success) {
        throw std::runtime_error("Failed to write to handle");
    }
    // MSDN seems to imply a successful WriteFile call always writes the entire buffer
    assert(written == data.size());
}

std::vector<unsigned char> HandleIO::read(size_t size, bool exact) {
    std::vector<unsigned char> data(size);
    if (exact) {
        read_full_into(data);
    } else {
        size_t read = read_into(data);
        data.resize(read);
    }
    return data;
}

void HandleIO::cancel_async() {
    CancelIo(source);
}

Sync HandleIO::read_async(std::span<unsigned char> data, DWORD *read) {
    Sync done = Sync::create_event(true);  // TODO: Maybe different parameters?

    OVERLAPPED ovl{.hEvent = done.raw()};
    bool success = ReadFile(
        source,
        data.data(),
        data.size(),
        read,
        &ovl
    );
    // TODO: Check success and GetLastError for IO pending

    return done;
}

Sync HandleIO::write_async(std::span<const unsigned char> data, DWORD *written) {
    Sync done = Sync::create_event(true);  // TODO: Maybe different parameters?

    OVERLAPPED ovl{.hEvent = done.raw()};
    bool success = WriteFile(
        source,
        data.data(),
        data.size(),
        written,
        &ovl
    );
    // TODO: Check success and GetLastError for IO pending

    return done;
}

}  // namespace abel
