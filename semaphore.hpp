#pragma once

#include "tickTimer.hpp"
#include "txCommon.hpp"
#include <cassert>
#include <functional>
#include <limits>
#include <string_view>

namespace ThreadX
{
template <Ulong Ceiling = std::numeric_limits<Ulong>::max(), Ulong InitialCount = 0>
class CountingSemaphore final : Native::TX_SEMAPHORE
{
    static_assert(InitialCount <= Ceiling);

  public:
    using NotifyCallback = std::function<void(CountingSemaphore &)>;

    // none copyable or movable
    CountingSemaphore(const CountingSemaphore &) = delete;
    CountingSemaphore &operator=(const CountingSemaphore &) = delete;

    ///
    constexpr auto max() const;

    /// Constructor
    /// \param name name of the semaphore.
    explicit CountingSemaphore(const std::string_view name, const NotifyCallback &releaseNotifyCallback = {});
    ~CountingSemaphore();

    /// attempts to retrieve an instance (a single count) from the specified counting semaphore.
    auto acquire();

    // must be used for calls from initialization, timers, and ISRs
    auto tryAcquire();

    template <class Clock, typename Duration>
    auto tryAcquireUntil(const std::chrono::time_point<Clock, Duration> &time);

    /// retrieves an instance (a single count) from the specified counting semaphore.
    /// As a result, the specified semaphore's count is decreased by one.
    /// \param duration
    template <typename Rep, typename Period>
    auto tryAcquireFor(const std::chrono::duration<Rep, Period> &duration);

    ///  puts a number of instances into the specified counting semaphore, which in reality increments the counting semaphore by
    ///  count value. If the counting semaphore's current value is greater than or equal to the specified ceiling, the instance
    ///  will not be put and a TX_CEILING_EXCEEDED error will be returned.
    /// \param count
    auto release(Ulong count);

    /// puts an instance into the specified counting semaphore, which in reality increments the counting semaphore by
    /// one. If the counting semaphore's current value is greater than or equal to the specified ceiling, the instance
    ///  will not be put and a TX_CEILING_EXCEEDED error will be returned.
    auto release();

    /// places the highest priority thread suspended for an instance of the semaphore at the front of the suspension
    /// list. All other threads remain in the same FIFO order they were suspended in.
    auto prioritise();

    /// returns the name of the semaphore.
    auto name() const;

    /// returns the current count of the semaphore.
    auto count() const;

  private:
    static auto releaseNotifyCallback(auto notifySemaphorePtr);

    const NotifyCallback m_releaseNotifyCallback;
};

template <Ulong Ceiling, Ulong InitialCount>
constexpr auto CountingSemaphore<Ceiling, InitialCount>::max() const
{
    return Ceiling;
}

/// Constructor
/// \param InitialCount
/// \param releaseNotifyCallback The Notifycallback is not allowed to call any ThreadX API with a suspension option.
template <Ulong Ceiling, Ulong InitialCount>
CountingSemaphore<Ceiling, InitialCount>::CountingSemaphore(const std::string_view name, const NotifyCallback &releaseNotifyCallback)
    : Native::TX_SEMAPHORE{}, m_releaseNotifyCallback{releaseNotifyCallback}
{
    using namespace Native;
    [[maybe_unused]] Error error{tx_semaphore_create(this, const_cast<char *>(name.data()), InitialCount)};
    assert(error == Error::success);

    if (releaseNotifyCallback)
    {
        error = Error{tx_semaphore_put_notify(this, CountingSemaphore::releaseNotifyCallback)};
        assert(error == Error::success);
    }
}

template <Ulong Ceiling, Ulong InitialCount>
CountingSemaphore<Ceiling, InitialCount>::~CountingSemaphore()
{
    [[maybe_unused]] Error error{tx_semaphore_delete(this)};
    assert(error == Error::success);
}

template <Ulong Ceiling, Ulong InitialCount>
auto CountingSemaphore<Ceiling, InitialCount>::acquire()
{
    return tryAcquireFor(TickTimer::waitForever);
}

template <Ulong Ceiling, Ulong InitialCount>
auto CountingSemaphore<Ceiling, InitialCount>::tryAcquire()
{
    return tryAcquireFor(TickTimer::noWait);
}

template <Ulong Ceiling, Ulong InitialCount>
template <class Clock, typename Duration>
auto CountingSemaphore<Ceiling, InitialCount>::tryAcquireUntil(const std::chrono::time_point<Clock, Duration> &time)
{
    return tryAcquireFor(time - Clock::now());
}

template <Ulong Ceiling, Ulong InitialCount>
template <typename Rep, typename Period>
auto CountingSemaphore<Ceiling, InitialCount>::tryAcquireFor(const std::chrono::duration<Rep, Period> &duration)
{
    return Error{tx_semaphore_get(this, TickTimer::ticks(duration))};
}

template <Ulong Ceiling, Ulong InitialCount>
auto CountingSemaphore<Ceiling, InitialCount>::release(Ulong count)
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

template <Ulong Ceiling, Ulong InitialCount>
auto CountingSemaphore<Ceiling, InitialCount>::release()
{
    return Error{tx_semaphore_ceiling_put(this, Ceiling)};
}

template <Ulong Ceiling, Ulong InitialCount>
auto CountingSemaphore<Ceiling, InitialCount>::prioritise()
{
    return Error{tx_semaphore_prioritize(this)};
}

template <Ulong Ceiling, Ulong InitialCount>
auto CountingSemaphore<Ceiling, InitialCount>::name() const
{
    return std::string_view{tx_semaphore_name};
}

template <Ulong Ceiling, Ulong InitialCount>
auto CountingSemaphore<Ceiling, InitialCount>::count() const
{
    return tx_semaphore_count;
}

template <Ulong Ceiling, Ulong InitialCount>
auto CountingSemaphore<Ceiling, InitialCount>::releaseNotifyCallback(auto notifySemaphorePtr)
{
    auto &semaphore{static_cast<CountingSemaphore &>(*notifySemaphorePtr)};
    semaphore.m_releaseNotifyCallback(semaphore);
}

template <Ulong InitialCount = 0>
using BinarySemaphore = CountingSemaphore<1, InitialCount>;
} // namespace ThreadX
