#pragma once

#include "tickTimer.hpp"
#include "txCommon.hpp"
#include <mutex>
#include <string_view>

#define tryLock() try_lock()
#define tryLockUntil(x) try_lock_until(x)
#define tryLockFor(x) try_lock_for(x)

namespace ThreadX
{
/// Inherit mode
enum class InheritMode : Uint
{
    noInherit, ///< noInherit
    inherit    ///< inherit
};

/// Mutex for locking access to resources.
class Mutex final : Native::TX_MUTEX
{
  public:
    /// \param inheritMode
    explicit Mutex(const InheritMode inheritMode = InheritMode::inherit);
    explicit Mutex(const std::string_view name, const InheritMode inheritMode = InheritMode::inherit);

    /// destructor
    ~Mutex();

    Mutex(const Mutex &) = delete;
    Mutex &operator=(const Mutex &) = delete;

    /// attempts to obtain exclusive ownership of the specified mutex. If the calling thread already owns the mutex, an
    /// internal counter is incremented and a successful status is returned.
    /// \param duration
    auto lock() -> Error;

    // must be used for calls from initialization, timers, and ISRs
    auto try_lock() -> Error;

    template <class Clock, typename Duration>
    auto try_lock_until(const std::chrono::time_point<Clock, Duration> &time) -> Error;

    template <typename Rep, typename Period>
    auto try_lock_for(const std::chrono::duration<Rep, Period> &duration) -> Error;

    /// decrements the ownership count of the specified mutex.
    /// If the ownership count is zero, the mutex is made available.
    auto unlock() -> Error;

    [[nodiscard]] auto name() const -> std::string_view;

    /// Places the highest priority thread suspended for ownership of the mutex at the front of the suspension list.
    /// All other threads remain in the same FIFO order they were suspended in.
    auto prioritise() -> Error;

    [[nodiscard]] auto lockingThreadID() const -> uintptr_t;
};

template <class Clock, typename Duration>
auto Mutex::try_lock_until(const std::chrono::time_point<Clock, Duration> &time) -> Error
{
    return try_lock_for(time - Clock::now());
}

template <typename Rep, typename Period>
auto Mutex::try_lock_for(const std::chrono::duration<Rep, Period> &duration) -> Error
{
    return Error{tx_mutex_get(this, TickTimer::ticks(duration))};
}

using LockGuard = std::lock_guard<Mutex>;
using UniqueLock = std::unique_lock<Mutex>;

using timedMutex = Mutex;
using recursiveMutex = Mutex;
using recursiveTimedMutex = Mutex;
} // namespace ThreadX
