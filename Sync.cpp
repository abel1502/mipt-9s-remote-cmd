#include "Sync.hpp"

#include <stdexcept>
#include <utility>

namespace abel {

Sync Sync::create_event(bool manualReset, bool initialState, bool inheritHandle) {
    SECURITY_ATTRIBUTES sa{
        .nLength = sizeof(SECURITY_ATTRIBUTES),
        .lpSecurityDescriptor = nullptr,
        .bInheritHandle = inheritHandle,
    };

    return Sync(CreateEvent(&sa, manualReset, initialState, nullptr)).validate();
}

Sync Sync::create_mutex(bool initialOwner, bool inheritHandle) {
    SECURITY_ATTRIBUTES sa{
        .nLength = sizeof(SECURITY_ATTRIBUTES),
        .lpSecurityDescriptor = nullptr,
        .bInheritHandle = inheritHandle,
    };

    return Sync(CreateMutex(&sa, initialOwner, nullptr)).validate();
}

bool Sync::is_set() const {
    return wait_timeout(0);
}

void Sync::wait() const {
    wait_timeout(INFINITE);
}

bool Sync::wait_timeout(DWORD miliseconds) const {
    DWORD result = WaitForSingleObject(raw(), miliseconds);
    switch (result) {
    case WAIT_OBJECT_0:
        return true;
    case WAIT_TIMEOUT:
        return false;
    case WAIT_FAILED:
        throw std::runtime_error("Failed to wait on handle");
    case WAIT_ABANDONED:
        throw std::runtime_error("Wait abandoned");
    default:
        throw std::runtime_error("Unknown wait result");
    }
}

}  // namespace abel
