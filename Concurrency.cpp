#include "Concurrency.hpp"

namespace abel {

void AIOEnv::update_current(std::coroutine_handle<> prev, std::coroutine_handle<> coro) noexcept {
    if (current_ != prev) {
        fail("Nonlinear use of AIOEnv detected");
    }
    current_ = coro;
}

void AIOEnv::step() {
    if (!current_.done() && io_done_.is_signaled()) {
        current_.resume();
    }
}

ParallelAIOs::ParallelAIOs(std::vector<AIO<void>> tasks) :
    tasks{std::move(tasks)},
    envs{std::make_unique<AIOEnv[]>(size())},
    events{std::make_unique<Handle[]>(size())} {

    for (size_t i = 0; i < size(); ++i) {
        envs[i].attach(tasks[i]);
        events[i] = envs[i].io_done();
    }
}

void ParallelAIOs::wait_any(DWORD miliseconds) {
    Handle::wait_multiple({events.get(), size()}, false, miliseconds);
}

void ParallelAIOs::step() {
    for (size_t i = 0; i < size(); ++i) {
        envs[i].step();
    }
}

bool ParallelAIOs::done() const {
    for (size_t i = 0; i < size(); ++i) {
        if (envs[i].current() != nullptr) {
            return false;
        }
    }
    return true;
}

void ParallelAIOs::run() {
    while (!done()) {
        wait_any();
        step();
    }
}

}  // namespace abel
