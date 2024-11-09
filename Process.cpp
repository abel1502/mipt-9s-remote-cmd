#include "Process.hpp"

#include <stdexcept>

namespace abel {

Process Process::create(
    const std::wstring &executable,
    const std::wstring &arguments,
    const std::wstring &workingDirectory,
    bool inheritHandles,
    DWORD creationFlags,
    DWORD startupFlags,
    HANDLE stdInput,
    HANDLE stdOutput,
    HANDLE stdError,
    std::function<void(STARTUPINFO &)> extraParams
) {
    PROCESS_INFORMATION processInfo{};

    std::wstring fullArgs{};
    fullArgs.reserve(arguments.size() + executable.size() + 2);
    fullArgs.append(executable);
    fullArgs.append(L" ");
    fullArgs.append(arguments);

    if (stdInput || stdOutput || stdError) {
        startupFlags |= STARTF_USESTDHANDLES;
    }

    STARTUPINFO startupInfo{
        .cb = sizeof(STARTUPINFO),
        .dwFlags = startupFlags,
        .hStdInput = stdInput,
        .hStdOutput = stdOutput,
        .hStdError = stdError,
    };

    if (extraParams) {
        extraParams(startupInfo);
    }

    bool success = CreateProcess(
        executable.c_str(),
        fullArgs.data(),
        nullptr,
        nullptr,
        inheritHandles,
        creationFlags,
        nullptr,
        workingDirectory.size() > 0 ? workingDirectory.c_str() : nullptr,
        &startupInfo,
        &processInfo
    );

    if (!success) {
        throw std::runtime_error("Failed to create process");
    }

    Handle process{processInfo.hProcess};
    Handle thread{processInfo.hThread};

    process.validate();
    thread.validate();

    Process result{};

    result.process = std::move(process);
    result.pid = processInfo.dwProcessId;
    result.thread = std::move(thread);
    result.tid = processInfo.dwThreadId;

    return result;
}

}  // namespace abel
