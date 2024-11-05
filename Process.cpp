#include "Process.hpp"

Process Process::create(
    std::wstring_view executable,
    std::wstring_view arguments,
    std::wstring_view workingDirectory,
    bool inheritHandles,
    DWORD creationFlags,
    DWORD startupFlags,
    Handle stdInput,
    Handle stdOutput,
    Handle stdError,
    std::function<void(STARTUPINFO &)> extraParams
) {
    PROCESS_INFORMATION processInfo{};

    std::wstring fullArgs{};
    fullArgs.reserve(arguments.size() + executable.size() + 1);
    fullArgs.append(executable);
    fullArgs.append(L" ");
    fullArgs.append(arguments);

    STARTUPINFO startupInfo{
        .cb = sizeof(STARTUPINFO),
        .dwFlags = startupFlags,
        .hStdInput = stdInput.raw(),
        .hStdOutput = stdOutput.raw(),
        .hStdError = stdError.raw(),
    };

    if (extraParams) {
        extraParams(startupInfo);
    }

    CreateProcess(
        executable.data(),
        fullArgs.data(),
        nullptr,
        nullptr,
        inheritHandles,
        creationFlags,
        nullptr,
        workingDirectory.data(),
        &startupInfo,
        &processInfo
    );

    Handle process{processInfo.hProcess};
    Handle thread{processInfo.hThread};

    process.validate();
    thread.validate();

    Process result{};

    result.process = std::move(process);
    result.pid = processInfo.dwProcessId;
    result.thread = std::move(thread);
    result.tid = processInfo.dwThreadId;
    result.stdInput = std::move(stdInput);
    result.stdOutput = std::move(stdOutput);
    result.stdError = std::move(stdError);

    return result;

}