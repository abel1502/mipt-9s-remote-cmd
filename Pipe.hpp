#pragma once

#include <Windows.h>
#include <utility>

#include "Handle.hpp"

namespace abel {

class Pipe {
public:
    Handle read{};
    Handle write{};

    constexpr Pipe() {
    }

    constexpr Pipe(Pipe &&other) noexcept = default;
    constexpr Pipe &operator=(Pipe &&other) noexcept = default;

    static Pipe create(bool inheritHandles = true, DWORD bufSize = 0);
};

}  // namespace abel
