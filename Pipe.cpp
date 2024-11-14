#include "Pipe.hpp"

#include "Error.hpp"

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
        fail("Failed to create pipe");
    }

    result.read.validate();
    result.write.validate();

    return result;
}

}  // namespace abel
