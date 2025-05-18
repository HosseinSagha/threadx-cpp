#pragma once

#include "tickTimer.hpp"
#include "txCommon.hpp"
#include <cassert>
#include <functional>
#include <limits>
#include <string_view>

namespace ThreadX
{
template <Ulong Ceiling = std::numeric_limits<Ulong>::max()>
class CountingSemaphore final : Native::TX_SEMAPHORE
{
  public:
    using NotifyCallback = std::function<void(CountingSemaphore &)>;

    // none copyable or movable
    CountingSemaphore(const CountingSemaphore &) = delete;
    CountingSemaphore &operator=(const CountingSemaphore &) = delete;

    ///
    [[nodiscard]] consteval auto max() const -> Ulong;

    /// Constructor
    /// \param name name of the semaphore.
    explicit CountingSemaphore(const std::string_view name, const Ulong initialCount = 0, const NotifyCallback &releaseNotifyCallback = {});
    ~CountingSemaphore();

    /// attempts to retrieve an instance (a single count) from the specified counting semaphore.
    auto acquire() -> Error;

    // must be used for calls from initialization, timers, and ISRs
    auto tryAcquire() -> Error;

    template <class Clock, typename Duration>
    auto tryAcquireUntil(const std::chrono::time_point<Clock, Duration> &time) -> Error;

    /// retrieves an instance (a single count) from the specified counting semaphore.
    /// As a result, the specified semaphore's count is decreased by one.
    /// \param duration
    template <typename Rep, typename Period>
    auto tryAcquireFor(const std::chrono::duration<Rep, Period> &duration) -> Error;

    ///  puts a number of instances into the specified counting semaphore, which in reality increments the counting semaphore by
    ///  count value. If the counting semaphore's current value is greater than or equal to the specified ceiling, the instance
    ///  will not be put and a TX_CEILING_EXCEEDED error will be returned.
    /// \param count
    auto release(Ulong count) -> Error;

    /// puts an instance into the specified counting semaphore, which in reality increments the counting semaphore by
    /// one. If the counting semaphore's current value is greater than or equal to the specified ceiling, the instance
    ///  will not be put and a TX_CEILING_EXCEEDED error will be returned.
    auto release() -> Error;

    /// places the highest priority thread suspended for an instance of the semaphore at the front of the suspension
    /// list. All other threads remain in the same FIFO order they were suspended in.
    auto prioritise() -> Error;

    /// returns the name of the semaphore.
    [[nodiscard]] auto name() const -> std::string_view;

    /// returns the current count of the semaphore.
    [[nodiscard]] auto count() const -> Ulong;

  private:
    static auto releaseNotifyCallback(auto notifySemaphorePtr) -> void;

    const NotifyCallback m_releaseNotifyCallback;
};

template <Ulong Ceiling>
consteval auto CountingSemaphore<Ceiling>::max() const -> Ulong
{
    return Ceiling;
}

/// Constructor
/// \param initialCount
/// \param releaseNotifyCallback The Notifycallback is not allowed to call any ThreadX API with a suspension option.
template <Ulong Ceiling>
CountingSemaphore<Ceiling>::CountingSemaphore(const std::string_view name, const Ulong initialCount, const NotifyCallback &releaseNotifyCallback)
    : Native::TX_SEMAPHORE{}, m_releaseNotifyCallback{releaseNotifyCallback}
{
    assert(initialCount <= Ceiling);

    using namespace Native;
    [[maybe_unused]] Error error{tx_semaphore_create(this, const_cast<char *>(name.data()), initialCount)};
    assert(error == Error::success);

    if (releaseNotifyCallback)
    {
        error = Error{tx_semaphore_put_notify(this, CountingSemaphore::releaseNotifyCallback)};
        assert(error == Error::success);
    }
}

template <Ulong Ceiling>
CountingSemaphore<Ceiling>::~CountingSemaphore()
{
    [[maybe_unused]] Error error{tx_semaphore_delete(this)};
    assert(error == Error::success);
}

template <Ulong Ceiling>
auto CountingSemaphore<Ceiling>::acquire() -> Error
{
    return tryAcquireFor(TickTimer::waitForever);
}

template <Ulong Ceiling>
auto CountingSemaphore<Ceiling>::tryAcquire() -> Error
{
    return tryAcquireFor(TickTimer::noWait);
}

template <Ulong Ceiling>
template <class Clock, typename Duration>
auto CountingSemaphore<Ceiling>::tryAcquireUntil(const std::chrono::time_point<Clock, Duration> &time) -> Error
{
    return tryAcquireFor(time - Clock::now());
}

template <Ulong Ceiling>
template <typename Rep, typename Period>
auto CountingSemaphore<Ceiling>::tryAcquireFor(const std::chrono::duration<Rep, Period> &duration) -> Error
{
    return Error{tx_semaphore_get(this, TickTimer::ticks(duration))};
}

template <Ulong Ceiling>
auto CountingSemaphore<Ceiling>::release(Ulong count) -> Error
{
    while (count > 0)
    {
        if (Error error{release()}; error != Error::success)
        {
            return error;
        }

        --count;
    }

    return Error::success;
}

template <Ulong Ceiling>
auto CountingSemaphore<Ceiling>::release() -> Error
{
    return Error{tx_semaphore_ceiling_put(this, Ceiling)};
}

template <Ulong Ceiling>
auto CountingSemaphore<Ceiling>::prioritise() -> Error
{
    return Error{tx_semaphore_prioritize(this)};
}

template <Ulong Ceiling>
auto CountingSemaphore<Ceiling>::name() const -> std::string_view
{
    return std::string_view{tx_semaphore_name};
}

template <Ulong Ceiling>
auto CountingSemaphore<Ceiling>::count() const -> Ulong
{
    return tx_semaphore_count;
}

template <Ulong Ceiling>
auto CountingSemaphore<Ceiling>::releaseNotifyCallback(auto notifySemaphorePtr) -> void
{
    auto &semaphore{static_cast<CountingSemaphore &>(*notifySemaphorePtr)};
    semaphore.m_releaseNotifyCallback(semaphore);
}

using BinarySemaphore = CountingSemaphore<1>;
} // namespace ThreadX
