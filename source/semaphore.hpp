#pragma once

#include "timer.hpp"
#include "txCommon.hpp"
#include <functional>

namespace ThreadX
{
///
template <class T> class Semaphore : protected Native::TX_SEMAPHORE
{
  public:
    ///
    using NotifyCallback = std::function<void(Semaphore &)>;

    // none copyable or movable
    Semaphore(const Semaphore &) = delete;
    Semaphore &operator=(const Semaphore &) = delete;

    auto acquire();

    // must be used for calls from initialization, timers, and ISRs
    auto tryAcquire();

    auto tryAcquireUntil(const TickTimer::TimePoint &timePoint);

    /// retrieves an instance (a single count) from the specified counting semaphore.
    /// As a result, the specified semaphore's count is decreased by one.
    /// \param waitDuration
    auto tryAcquireFor(const TickTimer::Duration &waitDuration);

    auto release();

    auto release(Ulong count);

    ///  puts an instance into the specified counting semaphore, which in reality increments the counting semaphore by
    ///  one. If the counting semaphore's current value is greater than or equal to the specified ceiling, the instance
    ///  will not be put and a TX_CEILING_EXCEEDED error will be returned.
    /// \param ceiling
    auto releaseBoundedTo(const Ulong ceiling);

    /// places the highest priority thread suspended for an instance of the semaphore at the front of the suspension
    /// list. All other threads remain in the same FIFO order they were suspended in.
    auto prioritise();

    auto count() const;

  protected:
    /// Constructor
    /// \param initialCount
    /// \param releaseNotifyCallback The Notifycallback is not allowed to call any ThreadX API with a suspension option.
    Semaphore(const Ulong initialCount = 0, const NotifyCallback &releaseNotifyCallback = {});

    ~Semaphore();

  private:
    static void releaseNotifyCallback(auto notifySemaphorePtr);

    const NotifyCallback m_releaseNotifyCallback;
};

template <typename T>
Semaphore<T>::Semaphore(const Ulong initialCount, const NotifyCallback &releaseNotifyCallback)
    : Native::TX_SEMAPHORE{}, m_releaseNotifyCallback{releaseNotifyCallback}
{
    using namespace Native;
    [[maybe_unused]] Error error{tx_semaphore_create(this, const_cast<char *>("semaphore"), initialCount)};
    assert(error == Error::success);

    if (m_releaseNotifyCallback)
    {
        error = Error{tx_semaphore_put_notify(this, Semaphore::releaseNotifyCallback)};
        assert(error == Error::success);
    }
}

template <typename T> Semaphore<T>::~Semaphore()
{
    tx_semaphore_delete(this);
}

template <typename T> auto Semaphore<T>::acquire()
{
    return tryAcquireFor(TickTimer::waitForever);
}

template <typename T> auto Semaphore<T>::tryAcquire()
{
    return tryAcquireFor(TickTimer::noWait);
}

template <typename T> auto Semaphore<T>::tryAcquireUntil(const TickTimer::TimePoint &timePoint)
{
    return tryAcquireFor(timePoint - TickTimer::now());
}

template <typename T> auto Semaphore<T>::tryAcquireFor(const TickTimer::Duration &waitDuration)
{
    return Error{tx_semaphore_get(this, TickTimer::ticks(waitDuration))};
}

template <typename T> auto Semaphore<T>::release()
{
    return static_cast<T *>(this)->releaseImpl();
}

template <typename T> auto Semaphore<T>::release(Ulong count)
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

template <typename T> auto Semaphore<T>::releaseBoundedTo(const Ulong ceiling)
{
    return Error{tx_semaphore_ceiling_put(this, ceiling)};
}

template <typename T> auto Semaphore<T>::prioritise()
{
    return Error{tx_semaphore_prioritize(this)};
}

template <typename T> auto Semaphore<T>::count() const
{
    return tx_semaphore_count;
}

template <typename T> void Semaphore<T>::releaseNotifyCallback(auto notifySemaphorePtr)
{
    auto &semaphore{static_cast<Semaphore &>(*notifySemaphorePtr)};
    semaphore.m_releaseNotifyCallback(semaphore);
}

template <Ulong CeilingValue> class BoundedSemaphore : public Semaphore<BoundedSemaphore<CeilingValue>>
{
    friend class Semaphore<BoundedSemaphore>;

  public:
    BoundedSemaphore(
        const Ulong initialCount = 0, const Semaphore<BoundedSemaphore>::NotifyCallback &releaseNotifyCallback = {});

    constexpr auto ceiling();

  private:
    auto releaseImpl();
};

template <Ulong CeilingValue>
BoundedSemaphore<CeilingValue>::BoundedSemaphore(
    const Ulong initialCount, const Semaphore<BoundedSemaphore>::NotifyCallback &releaseNotifyCallback)
    : Semaphore<BoundedSemaphore>{initialCount, releaseNotifyCallback}
{
    assert(initialCount <= CeilingValue);
}

template <Ulong CeilingValue> auto BoundedSemaphore<CeilingValue>::releaseImpl()
{
    return BoundedSemaphore::releaseBoundedTo(CeilingValue);
}

template <Ulong CeilingValue> constexpr auto BoundedSemaphore<CeilingValue>::ceiling()
{
    return CeilingValue;
}

using BinarySemaphore = BoundedSemaphore<1>;

class CountingSemaphore : public Semaphore<CountingSemaphore>
{
    friend class Semaphore<CountingSemaphore>;

  public:
    using Semaphore<CountingSemaphore>::Semaphore;

    /// puts an instance into the specified counting semaphore, which in reality increments the counting semaphore by
    /// one
  private:
    Error releaseImpl();
};
} // namespace ThreadX
