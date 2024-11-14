#include "Handle.hpp"
#include "HandleIO.hpp"

namespace abel {

Handle Handle::clone() const {
    Handle result{};

    bool success = DuplicateHandle(
        GetCurrentProcess(),
        value,
        GetCurrentProcess(),
        &result.value,
        0,
        false,
        DUPLICATE_SAME_ACCESS
    );

    if (!success) {
        throw std::runtime_error("Failed to duplicate handle");
    }

    return result;
}

#pragma region IO
HandleIO Handle::io() const {
    return HandleIO{value};
}

Owning<HandleIO, Handle> Handle::owning_io(this Handle self) {
    HandleIO io = self.io();
    return Owning<HandleIO, Handle>(std::move(io), std::move(self));
}
#pragma endregion IO

#pragma region Sync
Handle Handle::create_event(bool manualReset, bool initialState, bool inheritHandle) {
    SECURITY_ATTRIBUTES sa{
        .nLength = sizeof(SECURITY_ATTRIBUTES),
        .lpSecurityDescriptor = nullptr,
        .bInheritHandle = inheritHandle,
    };

    return Handle(CreateEvent(&sa, manualReset, initialState, nullptr)).validate();
}

Handle Handle::create_mutex(bool initialOwner, bool inheritHandle) {
    SECURITY_ATTRIBUTES sa{
        .nLength = sizeof(SECURITY_ATTRIBUTES),
        .lpSecurityDescriptor = nullptr,
        .bInheritHandle = inheritHandle,
    };

    return Handle(CreateMutex(&sa, initialOwner, nullptr)).validate();
}

bool Handle::is_set() const {
    return wait_timeout(0);
}

void Handle::wait() const {
    wait_timeout(INFINITE);
}

bool Handle::wait_timeout(DWORD miliseconds) const {
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
#pragma endregion Sync

}  // namespace abel
