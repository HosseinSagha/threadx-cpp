#pragma once

#include "kernel.hpp"
#include "memoryPool.hpp"
#include "semaphore.hpp"
#include "thisThread.hpp"
#include "tickTimer.hpp"
#include "txCommon.hpp"
#include <cassert>

namespace ThreadX
{
enum class ThreadState : Uint
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
enum class ThreadStartType : Uint
{
    dontStart, ///< dontStart
    autoStart  ///< autoStart
};

enum class ThreadNotifyCondition : Uint
{
    entry,
    exit
};

inline constexpr Uint defaultPriority{16}; ///
inline constexpr Ulong noTimeSlice{};
inline constexpr Ulong minimumStackSize{TX_MINIMUM_STACK};

template <class Pool>
class Thread : Native::TX_THREAD, Allocation<Pool>
{
  public:
    using ErrorCallback = std::function<void(Thread &)>;
    using NotifyCallback = std::function<void(Thread &, const ThreadNotifyCondition)>;
    using StackInfo = struct
    {
        Ulong size;
        Ulong used;
        Ulong maxUsed;
        Ulong maxUsedPercent;
    };

    Thread(const Thread &) = delete;
    Thread &operator=(const Thread &) = delete;

    ///
    /// \param stackErrorNotifyCallback
    /// \return
    static auto registerStackErrorNotifyCallback(const ErrorCallback &stackErrorNotifyCallback) -> Error;

    /// Constructor
    /// \param pool
    /// \param stackSize
    /// \param priority
    /// \param preamptionThresh
    /// \param timeSlice
    /// \param startType
    explicit Thread(const std::string_view name, Pool &pool, const Ulong stackSize = minimumStackSize, const NotifyCallback &entryExitNotifyCallback = {},
                    const Uint priority = defaultPriority, const Uint preamptionThresh = defaultPriority, const Ulong timeSlice = noTimeSlice,
                    const ThreadStartType startType = ThreadStartType::autoStart)
        requires(std::is_base_of_v<BytePoolBase, Pool>);

    explicit Thread(const std::string_view name, Pool &pool, const NotifyCallback &entryExitNotifyCallback = {}, const Uint priority = defaultPriority,
                    const Uint preamptionThresh = defaultPriority, const Ulong timeSlice = noTimeSlice,
                    const ThreadStartType startType = ThreadStartType::autoStart)
        requires(std::is_base_of_v<BlockPoolBase, Pool>);

    /// resumes or prepares for execution a thread that was previously suspended by a suspend() call.
    /// In addition, this service resumes threads that were created without an automatic start.
    auto resume() -> Error;

    /// suspends the specified application thread. A thread may call this service to suspend itself.
    auto suspend() -> Error;

    /// The thread must be in either a ThreadState::completed or ThreadState::terminated state for it to be reset.
    auto reset() -> Error;

    /// resets the specified thread to execute at the entry point defined at thread creation.
    /// The thread must be in either a ThreadState::completed or ThreadState::terminated state for it to be resetarted.
    /// The thread must be resumed for it to execute again.
    auto restart() -> Error;

    /// terminates the specified application thread regardless of whether the thread is suspended or not.
    /// A thread may call this service to terminate itself. After being terminated, the thread must be reset for it to execute again.
    auto terminate() -> Error;

    /// aborts sleep or any other object suspension of the specified thread.
    /// If the wait is aborted, a Error::waitAborted is returned from the service that the thread was waiting on.
    auto abortWait() -> Error;

    auto id() const -> ThisThread::ID;

    auto name() const -> std::string_view;

    auto state() const -> ThreadState;

    /// Changes preemption-threshold of application thread.
    /// \param preempt
    /// \return
    auto preemption(const auto preempt) -> Error;

    auto preemption() const -> Uint;

    /// Change priority of application thread.
    /// \param priority
    /// \return
    auto priority(const auto priority) -> Error;

    auto priority() const -> Uint;
    /// Changes time-slice of application thread.
    /// Using preemption-threshold disables time-slicing for the specified thread.
    /// \param timeSlice
    /// \return
    auto timeSlice(const auto timeSlice) -> Error;

    auto timeSlice() const -> Ulong;

    auto join() -> void;

    auto joinable() const -> bool;

    auto stackInfo() const -> StackInfo;

  protected:
    ~Thread();

  private:
    static auto entryFunction(Ulong thisPtr) -> void;
    static auto stackErrorNotifyCallback(Native::TX_THREAD *const threadPtr) -> void;
    static auto entryExitNotifyCallback(auto *const threadPtr, const auto condition) -> void;

    auto init(const std::string_view name, const ThreadX::Ulong stackSize, const Uint priority, const Uint preamptionThresh, const Ulong timeSlice,
              const ThreadStartType startType) -> void;

    virtual void entryCallback() = 0;

    static inline ErrorCallback m_stackErrorNotifyCallback;

    const NotifyCallback m_entryExitNotifyCallback;
    BinarySemaphore<> *m_exitSignalPtr{};
}; // namespace ThreadX

template <class Pool>
auto Thread<Pool>::registerStackErrorNotifyCallback(const ErrorCallback &stackErrorNotifyCallback) -> Error
{
    Error error{tx_thread_stack_error_notify(stackErrorNotifyCallback ? Thread::stackErrorNotifyCallback : nullptr)};
    if (error == Error::success)
    {
        m_stackErrorNotifyCallback = stackErrorNotifyCallback;
    }

    return error;
}

template <class Pool>
Thread<Pool>::Thread(const std::string_view name, Pool &pool, const Ulong stackSize, const NotifyCallback &entryExitNotifyCallback, const Uint priority,
                     const Uint preamptionThresh, const Ulong timeSlice, const ThreadStartType startType)
    requires(std::is_base_of_v<BytePoolBase, Pool>)
    : Native::TX_THREAD{}, Allocation<Pool>{pool, stackSize}, m_entryExitNotifyCallback{entryExitNotifyCallback}
{
    init(name, stackSize, priority, preamptionThresh, timeSlice, startType);
}

template <class Pool>
Thread<Pool>::Thread(const std::string_view name, Pool &pool, const NotifyCallback &entryExitNotifyCallback, const Uint priority, const Uint preamptionThresh,
                     const Ulong timeSlice, const ThreadStartType startType)
    requires(std::is_base_of_v<BlockPoolBase, Pool>)
    : Native::TX_THREAD{}, Allocation<Pool>{pool}, m_entryExitNotifyCallback{entryExitNotifyCallback}
{
    init(name, pool.blockSize(), priority, preamptionThresh, timeSlice, startType);
}

template <class Pool>
auto Thread<Pool>::init(const std::string_view name, const ThreadX::Ulong stackSize, const Uint priority, const Uint preamptionThresh, const Ulong timeSlice,
                        const ThreadStartType startType) -> void
{
    using namespace Native;
    [[maybe_unused]] Error error{tx_thread_create(this, const_cast<char *>(name.data()), entryFunction, reinterpret_cast<Ulong>(this), Allocation<Pool>::allocationPtr(),
                                                  stackSize, priority, preamptionThresh, timeSlice, std::to_underlying(startType))};
    assert(error == Error::success);

    error = Error{tx_thread_entry_exit_notify(this, Thread::entryExitNotifyCallback)};
    assert(error == Error::success);
}

template <class Pool>
Thread<Pool>::~Thread()
{
    [[maybe_unused]] Error error{tx_thread_terminate(this)};
    assert(error == Error::success);

    error = Error{tx_thread_delete(this)};
    assert(error == Error::success);
}

template <class Pool>
auto Thread<Pool>::resume() -> Error
{
    return Error{tx_thread_resume(this)};
}

template <class Pool>
auto Thread<Pool>::suspend() -> Error
{
    return Error{tx_thread_suspend(this)};
}

template <class Pool>
auto Thread<Pool>::reset() -> Error
{
    return Error{tx_thread_reset(this)};
}

template <class Pool>
auto Thread<Pool>::restart() -> Error
{
    if (auto error = Error{tx_thread_reset(this)}; error != Error::success)
    {
        return error;
    }

    return Error{tx_thread_resume(this)};
}

template <class Pool>
auto Thread<Pool>::terminate() -> Error
{
    return Error{tx_thread_terminate(this)};
}

template <class Pool>
auto Thread<Pool>::abortWait() -> Error
{
    return Error{tx_thread_wait_abort(this)};
}

template <class Pool>
auto Thread<Pool>::id() const -> ThisThread::ID
{
    return ThisThread::ID(static_cast<const Native::TX_THREAD *>(this));
}

template <class Pool>
auto Thread<Pool>::name() const -> std::string_view
{
    return std::string_view{tx_thread_name};
}

template <class Pool>
auto Thread<Pool>::state() const -> ThreadState
{
    return ThreadState{tx_thread_state};
}

template <class Pool>
auto Thread<Pool>::preemption(const auto preempt) -> Error
{
    Uint oldPreempt{};
    return Error{tx_thread_preemption_change(this, preempt, std::addressof(oldPreempt))};
}

template <class Pool>
auto Thread<Pool>::preemption() const -> Uint
{
    return tx_thread_user_preempt_threshold;
}

template <class Pool>
auto Thread<Pool>::priority(const auto priority) -> Error
{
    Uint oldPriority;
    return Error{tx_thread_priority_change(this, priority, std::addressof(oldPriority))};
}

template <class Pool>
auto Thread<Pool>::priority() const -> Uint
{
    return tx_thread_user_priority;
}

template <class Pool>
auto Thread<Pool>::timeSlice(const auto timeSlice) -> Error
{
    Ulong oldTimeSlice;
    return Error{tx_thread_time_slice_change(this, timeSlice, std::addressof(oldTimeSlice))};
}

template <class Pool>
auto Thread<Pool>::timeSlice() const -> Ulong
{
    return tx_thread_new_time_slice;
}

template <class Pool>
auto Thread<Pool>::join() -> void
{
    assert(not m_exitSignalPtr);
    BinarySemaphore exitSignal("join");

    {
        Kernel::CriticalSection cs; // do not allow any change in thread state until m_exitSignalPtr is assigned.

        if (not joinable()) // Thread becomes unjoinable just before entryExitNotifyCallback() is called.
        {
            return;
        }

        m_exitSignalPtr = std::addressof(exitSignal);
    }

    [[maybe_unused]] auto error{exitSignal.acquire()}; // wait for release by exit notify callback
    assert(error == Error::success or error == Error::waitAborted);

    m_exitSignalPtr = nullptr;
}

template <class Pool>
auto Thread<Pool>::joinable() const -> bool
{
    // wait on itself resource deadlock and wait on finished thread.
    auto threadState{state()};
    return id() != ThisThread::id() and threadState != ThreadState::completed and threadState != ThreadState::terminated;
}

template <class Pool>
auto Thread<Pool>::stackInfo() const -> StackInfo
{
    return StackInfo{.size = tx_thread_stack_size,
                     .used = uintptr_t(tx_thread_stack_end) - uintptr_t(tx_thread_stack_ptr) + 1,
                     .maxUsed = uintptr_t(tx_thread_stack_end) - uintptr_t(tx_thread_stack_highest_ptr) + 1,
                     .maxUsedPercent = (uintptr_t(tx_thread_stack_end) - uintptr_t(tx_thread_stack_highest_ptr) + 1) * 100 /
                                       tx_thread_stack_size}; // As a rule of thumb, keep this below 70%
}

template <class Pool>
auto Thread<Pool>::entryFunction(Ulong thisPtr) -> void
{
    reinterpret_cast<Thread *>(thisPtr)->entryCallback();
}

template <class Pool>
auto Thread<Pool>::stackErrorNotifyCallback(Native::TX_THREAD *const threadPtr) -> void
{
    auto &thread{static_cast<Thread &>(*threadPtr)};
    thread.m_stackErrorNotifyCallback(thread);
}

template <class Pool>
auto Thread<Pool>::entryExitNotifyCallback(auto *const threadPtr, const auto condition) -> void
{
    auto &thread{static_cast<Thread &>(*threadPtr)};
    auto notifyCondition{ThreadNotifyCondition{condition}};

    if (thread.m_entryExitNotifyCallback)
    {
        thread.m_entryExitNotifyCallback(thread, notifyCondition);
    }

    if (notifyCondition == ThreadNotifyCondition::exit)
    {
        if (thread.m_exitSignalPtr)
        {
            [[maybe_unused]] auto error{thread.m_exitSignalPtr->release()};
            assert(error == Error::success);
        }
    }
}
} // namespace ThreadX
