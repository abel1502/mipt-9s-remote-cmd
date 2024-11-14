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
        fail("Failed to duplicate handle");
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

bool Handle::is_signaled() const {
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
        fail("Failed to wait on handle");
    case WAIT_ABANDONED:
        fail("Wait abandoned");
    default:
        fail("Unknown wait result");
    }
}

size_t Handle::wait_multiple(std::span<const Handle *> handles, bool all, DWORD miliseconds) {
    std::unique_ptr<HANDLE[]> handlesArr = std::make_unique<HANDLE[]>(handles.size());
    for (size_t i = 0; i < handles.size(); ++i) {
        handlesArr[i] = handles[i]->raw();
    }

    DWORD result = WaitForMultipleObjects(handles.size(), handlesArr.get(), all, miliseconds);

    if (WAIT_OBJECT_0 <= result && result < WAIT_OBJECT_0 + handles.size()) {
        return result - WAIT_OBJECT_0;
    }

    if (WAIT_ABANDONED_0 <= result && result < WAIT_ABANDONED_0 + handles.size()) {
        fail("Wait abandoned");
    }

    switch (result) {
    case WAIT_TIMEOUT:
        return -1U;
    case WAIT_FAILED:
        fail("Failed to wait on handles");
    case WAIT_ABANDONED:
        fail("Wait abandoned");
    default:
        fail("Unknown wait result");
    }
}
#pragma endregion Sync

#pragma region Thread
void Handle::suspend_thread() const {
    DWORD result = SuspendThread(raw());

    if (result == -1) {
        fail("Failed to suspend thread");
    }
}

void Handle::resume_thread() const {
    DWORD result = ResumeThread(raw());

    if (result == -1) {
        fail("Failed to resume thread");
    }
}
#pragma endregion Thread

}  // namespace abel
