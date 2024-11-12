#pragma once

#include <Windows.h>
#include <exception>
#include <stdexcept>
#include <utility>
#include <vector>
#include <span>
#include <cstdint>
#include <optional>

#include "Owning.hpp"


namespace abel {

class HandleIO;

class Sync;

class Handle {
protected:
    HANDLE value;

public:
    constexpr Handle() noexcept : value(NULL) {}

    constexpr Handle(nullptr_t) noexcept : Handle() {}

    constexpr Handle(HANDLE value) noexcept : value(value) {}

    Handle(const Handle &other) = delete;

    Handle &operator=(const Handle &other) = delete;

    constexpr Handle(Handle &&other) noexcept : value(other.value) {
        other.value = NULL;
    }

    constexpr Handle &&operator=(Handle &&other) noexcept {
        value = other.value;
        other.value = NULL;
        return std::move(*this);
    }

    ~Handle() noexcept {
        if (value) {
            CloseHandle(value);
            value = NULL;
        }
    }

    constexpr operator bool() const noexcept {
        return value != NULL;
    }

    template <typename Self>
    constexpr Self &&validate(this Self &&self) {
        if (self.value == NULL || self.value == INVALID_HANDLE_VALUE) {
            throw std::runtime_error("Handle is invalid");
        }
        return std::forward<Self>(self);
    }

    constexpr HANDLE raw() const noexcept {
        return value;
    }

    template <typename Self>
    constexpr decltype(auto) raw_ptr(this Self &&self) noexcept {
        return &self.value;
    }

    Handle clone() const;

    HandleIO io() const;

    Owning<HandleIO, Handle> owning_io(this Handle self);

    Sync sync(this Handle self);

    // Strips the extra information from a handle, turning it back into a baseline one
    template <typename Self>
    constexpr Handle downgrade(this Self self) noexcept {
        return (Handle)std::move(self);
    }
};

}  // namespace abel
