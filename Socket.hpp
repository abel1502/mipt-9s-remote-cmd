#pragma once

#include "Error.hpp"

#include <WinSock2.h>
#include <Windows.h>

#pragma comment(lib, "Ws2_32.lib")
#pragma comment(lib, "Mswsock.lib")
#pragma comment(lib, "AdvApi32.lib")

namespace abel {

class Socket {
protected:
    SOCKET socket{INVALID_SOCKET};

public:

};

// An instance of this must be alive throughout the period sockets are intended to be used.
// This handles the WSAStartup and WSACleanup calls in a RAII-friendly way.
class SocketLibGuard {
protected:
    WSADATA wsadata{};

public:
    SocketLibGuard() {
        int result = WSAStartup(MAKEWORD(2, 2), &wsadata);
        if (result) {
            fail_ws("WSAStartup failed", result);
        }
        // TODO: Check version in wsadata
    }

    const WSADATA &info() const {
        return wsadata;
    }

    ~SocketLibGuard() {
        int result = WSACleanup();
        if (result) {
            fail_ws("WSACleanup failed", result);
        }
    }
};

}  // namespace abel
