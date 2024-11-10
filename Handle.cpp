#include "Handle.hpp"
#include "HandleIO.hpp"

namespace abel {

Handle Handle::clone() const {
    Handle result{};

    bool success = DuplicateHandle(
        GetCurrentProcess(),
        value,
        GetCurrentProcess(),
        &result.value,
        0,
        false,
        DUPLICATE_SAME_ACCESS
    );

    if (!success) {
        throw std::runtime_error("Failed to duplicate handle");
    }

    return result;
}

HandleIO Handle::io() const {
    return HandleIO{value};
}

Owning<HandleIO, Handle> Handle::owning_io(this Handle self) {
    return Owning<HandleIO, Handle>(self.io(), std::move(self));
}

//std::vector<std::uint8_t> Handle::read(size_t size, bool exact) const {
//    std::vector<std::uint8_t> data(size);
//    DWORD read = 0;
//
//    do {
//        bool success = ReadFile(
//            raw(),
//            data.data() + data.size() - size,
//            (DWORD)size,
//            &read,
//            nullptr
//        );
//
//        if (!success) {
//            throw std::runtime_error("Failed to read from handle");
//        }
//
//        size -= read;
//    } while (exact && size != 0 && read > 0);
//
//    return data;
//}
//
//size_t Handle::write(std::span<const std::uint8_t> data, bool exact) const {
//    size_t total = 0;
//
//    do {
//        DWORD written = 0;
//        bool success = WriteFile(
//            raw(),
//            data.data() + total,
//            (DWORD)data.size() - total,
//            &written,
//            nullptr
//        );
//
//        if (!success) {
//            throw std::runtime_error("Failed to write to handle");
//        }
//
//        total += written;
//    } while (exact && total != data.size());
//
//    return total;
//}

}  // namespace abel
