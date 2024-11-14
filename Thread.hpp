#pragma once

#include <Windows.h>
#include <utility>

#include "Handle.hpp"

namespace abel {

class Thread {
protected:
    Thread() {}

public:
    Handle handle{};
    DWORD tid{};

    constexpr Thread(Thread &&other) noexcept = default;
    constexpr Thread &operator=(Thread &&other) noexcept = default;

    static Thread create(
        LPTHREAD_START_ROUTINE func,
        void *param = nullptr,
        bool inheritHandles = false,
        bool startSuspended = false
    );
};

}  // namespace abel
