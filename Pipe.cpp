#include "Pipe.hpp"

#include <stdexcept>

namespace abel {

Pipe Pipe::create(bool inheritHandles, DWORD bufSize) {
    Pipe result{};

    SECURITY_ATTRIBUTES sa{
        .nLength = sizeof(SECURITY_ATTRIBUTES),
        .lpSecurityDescriptor = nullptr,
        .bInheritHandle = inheritHandles,
    };

    bool success = CreatePipe(
        result.read.raw_ptr(),
        result.write.raw_ptr(),
        &sa,
        bufSize
    );

    if (!success) {
        throw std::runtime_error("Failed to create pipe");
    }

    result.read.validate();
    result.write.validate();

    return result;
}

}  // namespace abel
