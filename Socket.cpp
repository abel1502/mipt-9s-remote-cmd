#include "Socket.hpp"

#include "Concurrency.hpp"

namespace abel {

eof<size_t> Socket::read_into(std::span<unsigned char> data) {
    int read = recv(socket, (char *)data.data(), (int)data.size(), 0);
    if (read == SOCKET_ERROR) {
        fail("Failed to read from socket");
    }

    return eof((size_t)read, read == 0);
}

eof<size_t> Socket::write_from(std::span<const unsigned char> data) {
    int written = send(socket, (const char *)data.data(), (int)data.size(), 0);
    if (written == SOCKET_ERROR) {
        fail("Failed to write to socket");
    }

    return eof((size_t)written, written == 0);
}

//void Socket::cancel_async();

// WinAPI promises that WSAOVERLAPPED is compatible with OVERLAPPED,
// but this verifies this assumption
static_assert(sizeof(WSAOVERLAPPED) == sizeof(OVERLAPPED));

AIO<eof<size_t>> Socket::read_async_into(std::span<unsigned char> data) {
    auto &env = *co_await current_env{};
    WSAOVERLAPPED *overlapped = (WSAOVERLAPPED *)env.overlapped();

    WSABUF wsabuf{.len = (ULONG)data.size(), .buf = (char *)data.data()};

    int status = WSARecv(
        socket,
        &wsabuf,
        1,
        nullptr,
        0,
        overlapped,
        nullptr
    );

    if (status == SOCKET_ERROR && WSAGetLastError() != WSA_IO_PENDING) {
        fail("Failed to initiate asynchronous read from socket");
    }

    co_await io_done_signaled{};

    DWORD transmitted = 0;
    bool success = WSAGetOverlappedResult(
        socket,
        overlapped,
        &transmitted,
        false,
        0
    );

    if (!success) {
        fail("Failed to get overlapped operation result");
    }

    // TODO: Perhaps a WSAGetLastError check is necessary instead?
    co_return eof((size_t)transmitted, transmitted == 0);
}

AIO<eof<size_t>> Socket::write_async_from(std::span<const unsigned char> data) {
    auto &env = *co_await current_env{};
    WSAOVERLAPPED *overlapped = (WSAOVERLAPPED *)env.overlapped();

    // Note: const violation is okay because WSASend mustn't write to this buffer
    WSABUF wsabuf{.len = (ULONG)data.size(), .buf = (char *)data.data()};

    int status = WSASend(
        socket,
        &wsabuf,
        1,
        nullptr,
        0,
        overlapped,
        nullptr
    );

    if (status == SOCKET_ERROR && WSAGetLastError() != WSA_IO_PENDING) {
        fail("Failed to initiate asynchronous read from socket");
    }

    co_await io_done_signaled{};

    DWORD transmitted = 0;
    bool success = WSAGetOverlappedResult(
        socket,
        overlapped,
        &transmitted,
        false,
        0
    );

    if (!success) {
        fail("Failed to get overlapped operation result");
    }

    // TODO: Perhaps a WSAGetLastError check is necessary instead?
    co_return eof((size_t)transmitted, transmitted == 0);
}

}  // namespace abel
