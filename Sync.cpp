#include "Sync.hpp"

#include <stdexcept>
#include <utility>

namespace abel {

Sync Sync::create_event(bool manualReset = false, bool initialState = false, bool inheritHandle = false) {
    Sync result{};

    SECURITY_ATTRIBUTES sa{
        .nLength = sizeof(SECURITY_ATTRIBUTES),
        .lpSecurityDescriptor = nullptr,
        .bInheritHandle = inheritHandle,
    };

    result.handle = Handle(CreateEvent(&sa, manualReset, initialState, nullptr)).validate();

    return result;
}

Sync Sync::create_mutex(bool initialOwner = false, bool inheritHandle = false) {
    Sync result{};

    SECURITY_ATTRIBUTES sa{
        .nLength = sizeof(SECURITY_ATTRIBUTES),
        .lpSecurityDescriptor = nullptr,
        .bInheritHandle = inheritHandle,
    };

    result.handle = Handle(CreateMutex(&sa, initialOwner, nullptr)).validate();

    return result;
}

}  // namespace abel
