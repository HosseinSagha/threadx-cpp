#pragma once

#include "tickTimer.hpp"
#include "txCommon.hpp"
#include <functional>

namespace ThreadX
{
///
class SemaphoreBase : protected Native::TX_SEMAPHORE
{
  public:
    // none copyable or movable
    SemaphoreBase(const SemaphoreBase &) = delete;
    SemaphoreBase &operator=(const SemaphoreBase &) = delete;

    Error acquire();

    // must be used for calls from initialization, timers, and ISRs
    Error tryAcquire();

    template <class Clock, typename Duration>
    auto tryAcquireUntil(const std::chrono::time_point<Clock, Duration> &time);

    /// retrieves an instance (a single count) from the specified counting semaphore.
    /// As a result, the specified semaphore's count is decreased by one.
    /// \param duration
    template <typename Rep, typename Period> auto tryAcquireFor(const std::chrono::duration<Rep, Period> &duration);

    /// places the highest priority thread suspended for an instance of the semaphore at the front of the suspension
    /// list. All other threads remain in the same FIFO order they were suspended in.
    Error prioritise();

    std::string_view name() const;

    Ulong count() const;

  protected:
    /// Constructor
    /// \param initialCount
    /// \param releaseNotifyCallback The Notifycallback is not allowed to call any ThreadX API with a suspension option.
    SemaphoreBase(const std::string_view name, const Ulong initialCount);

    ~SemaphoreBase();
};

template <class Clock, typename Duration>
auto SemaphoreBase::tryAcquireUntil(const std::chrono::time_point<Clock, Duration> &time)
{
    return tryAcquireFor(time - Clock::now());
}

template <typename Rep, typename Period>
auto SemaphoreBase::tryAcquireFor(const std::chrono::duration<Rep, Period> &duration)
{
    return Error{tx_semaphore_get(this, TickTimer::ticks(std::chrono::duration_cast<TickTimer::Duration>(duration)))};
}

template <Ulong CeilingValue = std::numeric_limits<Ulong>::max()> class CountingSemaphore : public SemaphoreBase
{
  public:
    using NotifyCallback = std::function<void(CountingSemaphore &)>;

    constexpr auto max() const;

    CountingSemaphore(
        const std::string_view name, const Ulong initialCount = 0, const NotifyCallback &releaseNotifyCallback = {});

    ///  puts an instance into the specified counting semaphore, which in reality increments the counting semaphore by
    ///  one. If the counting semaphore's current value is greater than or equal to the specified ceiling, the instance
    ///  will not be put and a TX_CEILING_EXCEEDED error will be returned.
    /// \param count
    Error release(const Ulong count = 1);

  private:
    const NotifyCallback m_releaseNotifyCallback;

    static void releaseNotifyCallback(auto notifySemaphorePtr);
};

template <Ulong CeilingValue> constexpr auto CountingSemaphore<CeilingValue>::max() const
{
    return CeilingValue;
}

template <Ulong CeilingValue>
CountingSemaphore<CeilingValue>::CountingSemaphore(
    const std::string_view name, const Ulong initialCount, const NotifyCallback &releaseNotifyCallback)
    : SemaphoreBase{name, initialCount}, m_releaseNotifyCallback{releaseNotifyCallback}
{
    assert(initialCount <= CeilingValue);

    if (m_releaseNotifyCallback)
    {
        [[maybe_unused]] Error error{tx_semaphore_put_notify(this, CountingSemaphore::releaseNotifyCallback)};
        assert(error == Error::success);
    }
}

template <Ulong CeilingValue> Error CountingSemaphore<CeilingValue>::release(Ulong count)
{
    while (count > 0)
    {
        if (Error error{tx_semaphore_ceiling_put(this, CeilingValue)}; error != Error::success)
        {
            return error;
        }

        --count;
    }

    return Error::success;
}

template <Ulong CeilingValue> void CountingSemaphore<CeilingValue>::releaseNotifyCallback(auto notifySemaphorePtr)
{
    auto &semaphore{static_cast<CountingSemaphore &>(*notifySemaphorePtr)};
    semaphore.m_releaseNotifyCallback(semaphore);
}

using BinarySemaphore = CountingSemaphore<1>;
} // namespace ThreadX
