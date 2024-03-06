#pragma once

#include "timer.hpp"
#include "txCommon.hpp"
#include <mutex>

#define tryLock() try_lock()
#define tryLockUntil(x) try_lock_until(x)
#define tryLockFor(x) try_lock_for(x)

namespace ThreadX
{
class Mutex;

using LockGuard = std::lock_guard<Mutex>;
using ScopedLock = std::scoped_lock<Mutex>;
using UniqueLock = std::unique_lock<Mutex>;

using timedMutex = Mutex;
using recursiveMutex = Mutex;
using recursiveTimedMutex = Mutex;

/// Inherit mode
enum class InheritMode : Uint
{
    noInherit, ///< noInherit
    inherit    ///< inherit
};

/// Mutex for locking access to resources.
class Mutex : Native::TX_MUTEX
{
  public:
    /// \param inheritMode
    Mutex(const InheritMode inheritMode = InheritMode::noInherit);

    /// destructor
    ~Mutex();

    Mutex(const Mutex &) = delete;
    Mutex &operator=(const Mutex &) = delete;

    /// attempts to obtain exclusive ownership of the specified mutex. If the calling thread already owns the mutex, an
    /// internal counter is incremented and a successful status is returned.
    /// \param waitDuration
    Error lock();

    // must be used for calls from initialization, timers, and ISRs
    Error try_lock();

    Error try_lock_until(const TickTimer::TimePoint &timePoint);

    Error try_lock_for(const TickTimer::Duration &duration);

    /// decrements the ownership count of the specified mutex.
    /// If the ownership count is zero, the mutex is made available.
    Error unlock();

    /// Places the highest priority thread suspended for ownership of the mutex at the front of the suspension list.
    /// All other threads remain in the same FIFO order they were suspended in.
    Error prioritise();

    uintptr_t lockingThreadID() const;
};
} // namespace ThreadX
