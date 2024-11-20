#pragma once

#include <Windows.h>
#include <utility>
#include <string>
#include <string_view>
#include <functional>

#include "Handle.hpp"

namespace abel {

class Process {
protected:
    Process() {
    }

public:
    OwningHandle process{};
    DWORD pid{};
    OwningHandle thread{};
    DWORD tid{};

    constexpr Process(Process &&other) noexcept = default;
    constexpr Process &operator=(Process &&other) noexcept = default;

    static Process create(
        const std::wstring &executable,
        const std::wstring &arguments = L"",
        const std::wstring &workingDirectory = L"",
        bool inheritHandles = false,
        DWORD creationFlags = 0,
        DWORD startupFlags = 0,
        HANDLE stdInput = NULL,
        HANDLE stdOutput = NULL,
        HANDLE stdError = NULL,
        std::function<void(STARTUPINFO &)> extraParams = nullptr
    );
};

}  // namespace abel
