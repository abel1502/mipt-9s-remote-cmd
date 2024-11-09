#pragma once

#include <Windows.h>
#include <streambuf>
#include <string>
#include <stdexcept>

#include "Handle.hpp"

namespace abel {

namespace impl {

template <typename CharT>
struct WinCharTraits : public std::char_traits<CharT> {
    using off_type = DWORD;
    using pos_type = DWORD;
};

}  // namespace impl

// TODO: I don't like this... Probably should resort to something manual instead.
template <typename CharT>
class HandleStreamBuf : public std::basic_streambuf<CharT, impl::WinCharTraits<CharT>> {
protected:
    HANDLE source;

    virtual int_type underflow() override {
        char_type result{};

        off_type cntRead = 0;
        bool success = ReadFile(
            source,
            &result,
            sizeof(char_type),
            &cntRead,
            nullptr
        );

        if (!success) {
            throw std::runtime_error("Failed to read from handle");
        }

        if (cntRead < 1) {
            return traits_type::eof();
        }

        return traits_type::to_int_type(result);
    }

    virtual int_type overflow(int_type ch = Traits::eof()) override {
    }
};

}  // namespace abel
