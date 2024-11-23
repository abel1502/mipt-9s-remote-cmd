#pragma once

#include "Handle.hpp"
#include "Error.hpp"

#include <Windows.h>
#include <utility>
#include <coroutine>
#include <vector>
#include <exception>
#include <expected>
#include <concepts>
#include <cassert>
#include <memory>

namespace abel {

#pragma region impl
template <typename T>
struct _impl_promise_return {
    std::expected<T, std::exception_ptr> result = std::unexpected{nullptr};

    void return_value(T value) {
        result = std::move(value);
    }

    void unhandled_exception() {
        result = std::unexpected{std::current_exception()};
    }

    T get_result() {
        if (result.has_value()) {
            return std::move(result).value();
        }
        std::rethrow_exception(std::move(result).error());
    }
};

template <>
struct _impl_promise_return<void> {
    std::exception_ptr result = nullptr;

    void return_void() {
        result = nullptr;
    }

    void unhandled_exception() {
        result = std::current_exception();
    }

    void get_result() {
        if (result) {
            std::rethrow_exception(result);
        }
    }
};
#pragma endregion impl

struct current_coro {
};

struct current_env {
};

struct io_done_signaled {
};

class AIOEnv {
protected:
    OwningHandle io_done_ = Handle::create_event(true, false);  // TODO: Different flags?
    OVERLAPPED overlapped_{.hEvent = io_done_.raw()};
    std::coroutine_handle<> current_{nullptr};

public:
    AIOEnv() = default;

    template <typename T>
    void attach(AIO<T> &aio) {
        aio.coro.promise().env = this;
        current_ = aio.coro;
    }

    AIOEnv(const AIOEnv &other) = delete;
    AIOEnv &operator=(const AIOEnv &other) = delete;
    AIOEnv(AIOEnv &&other) = delete;
    AIOEnv &operator=(AIOEnv &&other) = delete;

    Handle io_done() const noexcept {
        return io_done_;
    }

    OVERLAPPED *overlapped() noexcept {
        return &overlapped_;
    }

    std::coroutine_handle<> current() const noexcept {
        return current_;
    }

    void update_current(std::coroutine_handle<> prev, std::coroutine_handle<> coro) noexcept;

    void step();
};

// AIO is a coroutine object for simple asynchronous IO on WinAPI handles.
// It is also used as an awaitable for async IO primitives.
template <typename T = void>
class [[nodiscard]] AIO {
public:
    struct promise_type : public _impl_promise_return<T> {
        AIOEnv *env;
        std::coroutine_handle<> parent = nullptr;

        AIO get_return_object() {
            return AIO{coroutine_ptr::from_promise(*this)};
        }

        std::suspend_never initial_suspend() noexcept {
            return {};
        }

        auto final_suspend() noexcept {
            struct Awaiter {
                bool await_ready() noexcept {
                    return false;
                };

                std::coroutine_handle<> await_suspend(coroutine_ptr self) noexcept {
                    self.promise().env->update_current(self, self.promise().parent);
                    return self.promise().parent ? self.promise().parent : std::noop_coroutine();
                }

                void await_resume() noexcept {
                }
            };

            return Awaiter{};
        }

        auto await_transform(current_coro) {
            struct Awaiter {
                coroutine_ptr current{};

                bool await_ready() noexcept {
                    return false;
                }

                bool await_suspend(coroutine_ptr coro) noexcept {
                    current = coro;
                    return false;
                }

                coroutine_ptr await_resume() noexcept {
                    return current;
                }
            };

            return Awaiter{};
        }

        auto await_transform(current_env) {
            struct Awaiter {
                AIOEnv *env{};

                bool await_ready() noexcept {
                    return false;
                }

                bool await_suspend(coroutine_ptr) noexcept {
                    return false;
                }

                AIOEnv *await_resume() noexcept {
                    return env;
                }
            };

            return Awaiter{env};
        }

        auto await_transform(io_done_signaled) {
            struct Awaiter {
                AIOEnv *env;

                bool await_ready() noexcept {
                    return false;
                }

                void await_suspend(coroutine_ptr coro) {
                    // Just to verify we are the current coroutine
                    env->update_current(coro, coro);
                }

                void await_resume() {
                    env->io_done().reset();
                }
            };

            return Awaiter{env};
        }

        decltype(auto) await_transform(auto &&x) {
            return std::forward<decltype(x)>(x);
        }
    };

    using coroutine_ptr = std::coroutine_handle<promise_type>;

protected:
    coroutine_ptr coro;

    friend AIOEnv;

public:
    explicit AIO(coroutine_ptr coro) :
        coro{coro} {
    }

    AIO(const AIO &other) = delete;
    AIO &operator=(const AIO &other) = delete;

    constexpr AIO(AIO &&other) :
        coro{std::move(other.coro)} {
        other.coro = nullptr;
    }

    AIO &operator=(AIO &&other) {
        std::swap(coro, other.coro);
    }

    ~AIO() {
        if (coro) {
            coro.destroy();
        }
        coro = nullptr;
    }

    [[nodiscard]] OwningHandle init() {
        OwningHandle io_done = Handle::create_event(true, false);
        coro.promise().init(io_done);
        return io_done;
    }

    void step() {
        // TODO: ?
        coro.resume();
    }

    //Handle io_done() const {
    //    return coro.promise().io_done;
    //}

    //OVERLAPPED *overlapped() const {
    //    return &coro.promise().overlapped;
    //}

    bool await_ready() noexcept {
        return coro.done();
    }

    template <typename U>
    void await_suspend(std::coroutine_handle<U> master) noexcept {
        auto &self_promise = coro.promise();
        auto &master_promise = master.promise();
        self_promise.env = master_promise.env;
        self_promise.parent = master;
        self_promise.env->update_current(master, coro);

        coro.resume();

        // TODO: Don't return control to master until we've returned a value
    }

    T await_resume() {
        // TODO: ?

        return coro.promise().get_result();
    }
};

class ParallelAIOs {
protected:
    std::vector<AIO<void>> tasks;
    std::unique_ptr<AIOEnv[]> envs;
    std::unique_ptr<Handle[]> events;

public:
    ParallelAIOs(std::vector<AIO<void>> tasks);

    size_t size() const {
        return tasks.size();
    }

    void wait_any(DWORD miliseconds = INFINITE);

    void step();

    bool done() const;

    void run();
};

}  // namespace abel
