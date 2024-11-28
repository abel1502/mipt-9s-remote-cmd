#include "Socket.hpp"

#include "Concurrency.hpp"

namespace abel {

OwningSocket Socket::create() {
    return OwningSocket(WSASocketA(AF_INET, SOCK_STREAM, 0, nullptr, 0, WSA_FLAG_OVERLAPPED)).validate();
}

OwningSocket Socket::connect(std::string host, uint16_t port) {
    OwningSocket result = Socket::create();

    timeval timeout{.tv_sec = 15, .tv_usec = 0};
    int status = WSAConnectByNameA(result.raw(), host.c_str(), std::to_string(port).c_str(), nullptr, nullptr, nullptr, nullptr, &timeout, nullptr);
    if (status == SOCKET_ERROR) {
        fail_ws("Failed to connect to socket");
    }

    return result;
}

OwningSocket Socket::listen(uint16_t port) {
    OwningSocket result = Socket::create();

    sockaddr_in addr{
        .sin_family = AF_INET,
        .sin_port = htons(port),
        .sin_addr = INADDR_ANY,
    };

    int status = ::bind(result.raw(), (sockaddr *)&addr, sizeof(addr));
    if (status == SOCKET_ERROR) {
        fail_ws("Failed to bind socket");
    }

    status = ::listen(result.raw(), SOMAXCONN);
    if (status == SOCKET_ERROR) {
        fail_ws("Failed to listen on socket");
    }

    return result;
}

OwningSocket Socket::accept() {
    return OwningSocket(::accept(raw(), nullptr, nullptr)).validate();
}

eof<size_t> Socket::read_into(std::span<unsigned char> data) {
    int read = ::recv(raw(), (char *)data.data(), (int)data.size(), 0);
    if (read == SOCKET_ERROR) {
        fail_ws("Failed to read from socket");
    }

    return eof((size_t)read, read == 0);
}

eof<size_t> Socket::write_from(std::span<const unsigned char> data) {
    int written = ::send(raw(), (const char *)data.data(), (int)data.size(), 0);
    if (written == SOCKET_ERROR) {
        fail_ws("Failed to write to socket");
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
        raw(),
        &wsabuf,
        1,
        nullptr,
        0,
        overlapped,
        nullptr
    );

    if (status == SOCKET_ERROR && WSAGetLastError() != WSA_IO_PENDING) {
        fail_ws("Failed to initiate asynchronous read from socket");
    }

    co_await io_done_signaled{};

    DWORD transmitted = 0;
    bool success = WSAGetOverlappedResult(
        raw(),
        overlapped,
        &transmitted,
        false,
        0
    );

    if (!success) {
        fail_ws("Failed to get overlapped operation result");
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
        raw(),
        &wsabuf,
        1,
        nullptr,
        0,
        overlapped,
        nullptr
    );

    if (status == SOCKET_ERROR && WSAGetLastError() != WSA_IO_PENDING) {
        fail_ws("Failed to initiate asynchronous read from socket");
    }

    co_await io_done_signaled{};

    DWORD transmitted = 0;
    bool success = WSAGetOverlappedResult(
        raw(),
        overlapped,
        &transmitted,
        false,
        0
    );

    if (!success) {
        fail_ws("Failed to get overlapped operation result");
    }

    // TODO: Perhaps a WSAGetLastError check is necessary instead?
    co_return eof((size_t)transmitted, transmitted == 0);
}

}  // namespace abel
