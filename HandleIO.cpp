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

size_t HandleIO::Future::get_result(DWORD miliseconds) {
    DWORD transmitted = 0;
    bool success = GetOverlappedResultEx(
        source,
        overlapped.get(),
        &transmitted,
        miliseconds,
        false
    );

    if (!success) {
        throw std::runtime_error("Failed to get overlapped operation result");
    }

    // TODO: Perhaps a GetLastError check is necessary instead?
    eof = (transmitted == 0);

    return transmitted;
}

HandleIO::Future HandleIO::read_async(std::span<unsigned char> data, Handle doneEvent) {
    Future result{source, std::move(doneEvent)};

    bool success = ReadFile(
        source,
        data.data(),
        data.size(),
        nullptr,
        result.overlapped.get()
    );

    if (!success && GetLastError() != ERROR_IO_PENDING) {
        throw std::runtime_error("Failed to initiate asynchronous read from handle");
    }

    return result;
}

HandleIO::Future HandleIO::write_async(std::span<const unsigned char> data, Handle doneEvent) {
    Future result{source, std::move(doneEvent)};

    bool success = WriteFile(
        source,
        data.data(),
        data.size(),
        nullptr,
        result.overlapped.get()
    );

    if (!success && GetLastError() != ERROR_IO_PENDING) {
        throw std::runtime_error("Failed to initiate asynchronous write to handle");
    }

    return result;
}

}  // namespace abel
