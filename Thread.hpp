#pragma once

#include <Windows.h>
#include <utility>
#include <concepts>
#include <functional>
#include <memory>

#include "Handle.hpp"
#include "Owning.hpp"

namespace abel {

class Thread {
protected:
    Thread() {
    }

public:
    OwningHandle handle{};
    DWORD tid{};

    constexpr Thread(Thread &&other) noexcept = default;
    constexpr Thread &operator=(Thread &&other) noexcept = default;

    static Thread create(
        LPTHREAD_START_ROUTINE func,
        void *param = nullptr,
        bool inheritHandles = false,
        bool startSuspended = false
    );

    template <std::invocable<> F>
        requires std::same_as<std::invoke_result_t<F>, DWORD>
    [[nodiscard]] static Owning<Thread, std::unique_ptr<F>> create(
        F &&func,
        bool inheritHandles = false,
        bool startSuspended = false
    ) {
        std::unique_ptr<F> funcPtr = std::make_unique(std::forward<F>(func));
        return Owning(
            create(
                [](void *arg) -> DWORD { return std::invoke(*reinterpret_cast<F *>(arg)); },
                funcPtr.get()
            ),
            std::move(funcPtr),
        );
    }
};

}  // namespace abel
