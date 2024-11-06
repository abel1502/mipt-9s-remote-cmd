#pragma once

#include <Windows.h>
#include <utility>

#include "Handle.hpp"

namespace abel {

class Pipe {
protected:
    Pipe() {}
public:
    Handle read{};
    Handle write{};

    constexpr Pipe(Pipe &&other) noexcept = default;
    constexpr Pipe &operator=(Pipe &&other) noexcept = default;

    static Pipe create(bool inheritHandles = true, DWORD bufSize = 0);
};

}  // namespace abel
