#pragma once

#include "tickTimer.hpp"
#include <chrono>

namespace ThreadX::ThisThread
{
using ID = uintptr_t;

auto id() -> ID;

/// relinquishes processor control to other ready-to-run threads at the same or higher priority
auto yield() -> void;

auto terminate() -> Error;

auto suspend() -> Error;

auto name() -> std::string_view;

template <class Clock, typename Duration>
auto sleepUntil(const std::chrono::time_point<Clock, Duration> &time) -> Error
{
    return sleepFor(time - Clock::now());
}

/// causes the calling thread to suspend for the specified time
/// \param duration
template <typename Rep, typename Period>
auto sleepFor(const std::chrono::duration<Rep, Period> &duration) -> Error
{
    return Error{Native::tx_thread_sleep(TickTimer::ticks(duration))};
}
}; // namespace ThreadX::ThisThread
