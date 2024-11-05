#pragma once

#include <Windows.h>
#include <exception>
#include <stdexcept>
#include <utility>

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

    inline ~Handle() noexcept {
        if (value) {
            CloseHandle(value);
            value = NULL;
        }
    }

    constexpr const Handle &validate() const &{
        return const_cast<Handle &>(*this).validate();
    }

    constexpr Handle &validate() &{
        if (value == NULL || value == INVALID_HANDLE_VALUE) {
            throw std::runtime_error("Handle is invalid");
        }
        return *this;
    }

    constexpr Handle &&validate() &&{
        return std::move(static_cast<Handle &>(*this).validate());
    }

    constexpr HANDLE raw() const noexcept {
        return value;
    }

    Handle clone() const;
};