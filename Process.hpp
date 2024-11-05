#pragma once

#include <Windows.h>
#include <exception>
#include <stdexcept>
#include <utility>
#include <string_view>
#include <functional>

#include "Handle.hpp"

class Process {
protected:
    Handle process{};
    DWORD pid{};
    Handle thread{};
    DWORD tid{};
    Handle stdInput{};
    Handle stdOutput{};
    Handle stdError{};

    Process() {}

public:
    constexpr Process(Process &&other) noexcept = default;
    constexpr Process &operator=(Process &&other) noexcept = default;

    static Process create(
        std::wstring_view executable,
        std::wstring_view arguments = L"",
        std::wstring_view workingDirectory = nullptr,
        bool inheritHandles = false,
        DWORD creationFlags = 0,
        DWORD startupFlags = 0,
        Handle stdInput = nullptr,
        Handle stdOutput = nullptr,
        Handle stdError = nullptr,
        std::function<void(STARTUPINFO &)> extraParams = nullptr
    );
};