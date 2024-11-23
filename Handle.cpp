#include "Handle.hpp"

#include <cassert>

#include "Error.hpp"
#include "Concurrency.hpp"

namespace abel {

void Handle::close() {
    bool success = CloseHandle(value);
    value = NULL;

    if (!success) {
        fail("Failed to close handle");
    }
}

OwningHandle Handle::clone() const {
    OwningHandle result{};

    bool success = DuplicateHandle(
        GetCurrentProcess(),
        value,
        GetCurrentProcess(),
        result.raw_ptr(),
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
#pragma region Synchronous
size_t Handle::read_into(std::span<unsigned char> data) {
    DWORD read = 0;
    bool success = ReadFile(raw(), data.data(), (DWORD)data.size(), &read, nullptr);
    if (!success) {
        fail("Failed to read from handle");
    }

    // TODO: EOF
    // eof = read == 0;

    return read;
}

void Handle::read_full_into(std::span<unsigned char> data) {
    // TODO: EOF
    while (data.size() > 0 /*&& !eof*/) {
        size_t read = read_into(data);
        data = data.subspan(read);
    }

    // TODO: EOF
    if (/*eof*/ false) {
        fail("End of stream reached prematurely");
    }
}

void Handle::write_from(std::span<const unsigned char> data) {
    DWORD written = 0;
    bool success = WriteFile(raw(), data.data(), (DWORD)data.size(), &written, nullptr);
    if (!success) {
        fail("Failed to write to handle");
    }
    // MSDN seems to imply a successful WriteFile call always writes the entire buffer
    assert(written == data.size());
}

std::vector<unsigned char> Handle::read(size_t size, bool exact) {
    std::vector<unsigned char> data(size);
    if (exact) {
        read_full_into(data);
    } else {
        size_t read = read_into(data);
        data.resize(read);
    }
    return data;
}
#pragma endregion Synchronous

#pragma region Asynchronous
void Handle::cancel_async() {
    CancelIo(raw());
}

AIO<size_t> Handle::read_async(std::span<unsigned char> data) {
    auto &env = *co_await current_env{};
    OVERLAPPED *overlapped = env.overlapped();

    bool success = ReadFile(
        raw(),
        data.data(),
        (DWORD)data.size(),
        nullptr,
        overlapped
    );

    if (!success && GetLastError() != ERROR_IO_PENDING) {
        fail("Failed to initiate asynchronous read from handle");
    }

    co_await io_done_signaled{};

    DWORD transmitted = 0;
    success = GetOverlappedResultEx(
        raw(),
        overlapped,
        &transmitted,
        0,
        false
    );

    if (!success) {
        fail("Failed to get overlapped operation result");
    }

    // TODO: EOF
    // TODO: Perhaps a GetLastError check is necessary instead?
    //eof = (transmitted == 0);

    co_return transmitted;
}

AIO<void> Handle::write_async(std::span<const unsigned char> data) {
    auto &env = *co_await current_env{};
    OVERLAPPED *overlapped = env.overlapped();

    bool success = WriteFile(
        raw(),
        data.data(),
        (DWORD)data.size(),
        nullptr,
        overlapped
    );

    if (!success && GetLastError() != ERROR_IO_PENDING) {
        fail("Failed to initiate asynchronous write to handle");
    }

    co_await io_done_signaled{};

    DWORD transmitted = 0;
    success = GetOverlappedResultEx(
        raw(),
        overlapped,
        &transmitted,
        0,
        false
    );

    if (!success) {
        fail("Failed to get overlapped operation result");
    }

    // TODO: EOF
    // TODO: Perhaps a GetLastError check is necessary instead?
    // eof = (transmitted == 0);

    co_return;
}
#pragma endregion Asynchronous
#pragma endregion IO

#pragma region Synchronization
OwningHandle Handle::create_event(bool manualReset, bool initialState, bool inheritHandle) {
    SECURITY_ATTRIBUTES sa{
        .nLength = sizeof(SECURITY_ATTRIBUTES),
        .lpSecurityDescriptor = nullptr,
        .bInheritHandle = inheritHandle,
    };

    return OwningHandle(CreateEvent(&sa, manualReset, initialState, nullptr)).validate();
}

OwningHandle Handle::create_mutex(bool initialOwner, bool inheritHandle) {
    SECURITY_ATTRIBUTES sa{
        .nLength = sizeof(SECURITY_ATTRIBUTES),
        .lpSecurityDescriptor = nullptr,
        .bInheritHandle = inheritHandle,
    };

    return OwningHandle(CreateMutex(&sa, initialOwner, nullptr)).validate();
}

void Handle::signal() {
    bool success = SetEvent(raw());
    if (!success) {
        fail("Failed to signal event");
    }
}

void Handle::reset() {
    bool success = ResetEvent(raw());
    if (!success) {
        fail("Failed to reset event");
    }
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

size_t Handle::wait_multiple(std::span<Handle> handles, bool all, DWORD miliseconds) {
    std::unique_ptr<HANDLE[]> handlesArr = std::make_unique<HANDLE[]>(handles.size());
    for (size_t i = 0; i < handles.size(); ++i) {
        handlesArr[i] = handles[i].raw();
    }

    DWORD result = WaitForMultipleObjects((DWORD)handles.size(), handlesArr.get(), all, miliseconds);

    if (WAIT_OBJECT_0 <= result && result < WAIT_OBJECT_0 + handles.size()) {
        return result - WAIT_OBJECT_0;
    }

    if (WAIT_ABANDONED_0 <= result && result < WAIT_ABANDONED_0 + handles.size()) {
        fail("Wait abandoned");
    }

    switch (result) {
    case WAIT_TIMEOUT:
        return (size_t)-1;
    case WAIT_FAILED:
        fail("Failed to wait on handles");
    case WAIT_ABANDONED:
        fail("Wait abandoned");
    default:
        fail("Unknown wait result");
    }
}
#pragma endregion Synchronization

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
