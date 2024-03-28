#pragma once

#include "memoryPool.hpp"
#include "mutex.hpp"
#include "semaphore.hpp"
#include "tickTimer.hpp"
#include "txCommon.hpp"
#include <functional>

namespace ThreadX
{
/// pure vitual class to inherit application threads from
class Thread : Native::TX_THREAD
{
  public:
    ///
    enum class State : Uint
    {
        ready,
        completed,
        terminated,
        suspended,
        sleep,
        queueSusp,
        SemaphoreSusp,
        evenyFlag,
        blockMemory,
        byteMemory,
        ioDriver,
        file,
        tcpIP,
        mutexSusp,
        priorityChange
    };

    ///
    enum class StartType : Uint
    {
        dontStart, ///< dontStart
        autoStart  ///< autoStart
    };

    enum class NotifyCondition : Uint
    {
        entry,
        exit
    };

    using NotifyCallback = std::function<void(Thread &, const NotifyCondition)>;
    using ErrorCallback = std::function<void(Thread &)>;
    using UintPair = std::pair<Error, Uint>;
    using UlongPair = std::pair<Error, Ulong>;
    using ID = uintptr_t;
    using StackInfo = struct
    {
        Ulong size;
        Ulong used;
        Ulong maxUsed;
        Ulong maxUsedPercent;
    };

    static constexpr Uint defaultPriority{16}; ///
    static constexpr Uint lowestPriority{TX_MAX_PRIORITIES - 1};
    static constexpr Ulong noTimeSlice{}; ///
    static constexpr Ulong minimumStackSize{TX_MINIMUM_STACK};

    /// Constructor
    /// \param pool
    /// \param stackSize
    /// \param priority
    /// \param preamptionThresh
    /// \param timeSlice
    /// \param startType
    Thread(const std::string_view name, BytePoolBase &pool, const Ulong stackSize = minimumStackSize,
           const NotifyCallback &entryExitNotifyCallback = {}, const Uint priority = defaultPriority,
           const Uint preamptionThresh = defaultPriority, const Ulong timeSlice = noTimeSlice,
           const StartType startType = StartType::autoStart);

    Thread(const std::string_view name, BlockPoolBase &pool, const NotifyCallback &entryExitNotifyCallback = {},
           const Uint priority = defaultPriority, const Uint preamptionThresh = defaultPriority,
           const Ulong timeSlice = noTimeSlice, const StartType startType = StartType::autoStart);

    Thread(const Thread &) = delete;
    Thread &operator=(const Thread &) = delete;

    ///
    /// \param stackErrorNotifyCallback
    /// \return
    static Error registerStackErrorNotifyCallback(const ErrorCallback &stackErrorNotifyCallback);

    /// resumes or prepares for execution a thread that was previously suspended by a suspend() call.
    /// In addition, this service resumes threads that were created without an automatic start.
    Error resume();

    /// suspends the specified application thread. A thread may call this service to suspend itself.
    Error suspend();

    /// resets the specified thread to execute at the entry point defined at thread creation.
    /// The thread must be in either a State::completed or State::terminated state for it to be reset.
    /// The thread must be resumed for it to execute again.
    Error restart();

    /// terminates the specified application thread regardless of whether the thread is suspended or not.
    /// A thread may call this service to terminate itself. After being terminated, the thread must be reset for it to execute again.
    Error terminate();

    /// aborts sleep or any other object suspension of the specified thread.
    /// If the wait is aborted, a Error::waitAborted is returned from the service that the thread was waiting on.
    Error abortWait();

    uintptr_t id();

    std::string_view name();

    State state() const;

    /// Changes preemption-threshold of application thread.
    /// \param newPreempt
    /// \return
    UintPair preemption(const auto newPreempt);

    Uint preemption();

    /// Change priority of application thread.
    /// \param newPriority
    /// \return
    UintPair priority(const auto newPriority);

    Uint priority();

    /// Changes time-slice of application thread.
    /// Using preemption-threshold disables time-slicing for the specified thread.
    /// \param newTimeSlice
    /// \return
    UlongPair timeSlice(const auto newTimeSlice);

    void join();

    bool joinable();

    StackInfo stackInfo();

  protected:
    virtual ~Thread();

  private:
    static void entryFunction(auto thisPtr);
    static void entryExitNotifyCallback(auto *const threadPtr, const auto condition);
    static void stackErrorNotifyCallback(Native::TX_THREAD *const threadPtr);
    virtual void entryCallback() = 0; // pure virtual class

    static inline ErrorCallback m_stackErrorNotifyCallback;
    MemoryPoolBase &m_pool;
    const NotifyCallback m_entryExitNotifyCallback;
    BinarySemaphore *m_exitSignalPtr{};
};
} // namespace ThreadX

namespace ThreadX::ThisThread
{
uintptr_t id();

/// relinquishes processor control to other ready-to-run threads at the same or higher priority
void yield();

template <class Clock, typename Duration> auto sleepUntil(const std::chrono::time_point<Clock, Duration> &time)
{
    return sleepFor(time - Clock::now());
}

/// causes the calling thread to suspend for the specified time
/// \param duration
template <typename Rep, typename Period> auto sleepFor(const std::chrono::duration<Rep, Period> &duration)
{
    return Error{Native::tx_thread_sleep(TickTimer::ticks(std::chrono::duration_cast<TickTimer::Duration>(duration)))};
}
}; // namespace ThreadX::ThisThread
