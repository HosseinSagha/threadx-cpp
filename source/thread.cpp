#include "thread.hpp"
#include "kernel.hpp"
#include <utility>

namespace ThreadX
{
ThreadBase::ThreadBase(const NotifyCallback &entryExitNotifyCallback)
    : Native::TX_THREAD{}, m_entryExitNotifyCallback{entryExitNotifyCallback}
{
}

ThreadBase::~ThreadBase()
{
    terminate();
    tx_thread_delete(this);
};

Error ThreadBase::registerStackErrorNotifyCallback(const ErrorCallback &stackErrorNotifyCallback)
{
    Error error{
        tx_thread_stack_error_notify(stackErrorNotifyCallback ? ThreadBase::stackErrorNotifyCallback : nullptr)};
    if (error == Error::success)
    {
        m_stackErrorNotifyCallback = stackErrorNotifyCallback;
    }

    return error;
}

void ThreadBase::create(const std::string_view name, void *stackPtr, Ulong stackSize, const Uint priority,
                        const Uint preamptionThresh, const Ulong timeSlice, const StartType startType)
{
    using namespace Native;
    [[maybe_unused]] Error error{tx_thread_create(
        this, const_cast<char *>(name.data()), entryFunction, reinterpret_cast<Ulong>(this), stackPtr, stackSize,
        priority, preamptionThresh, timeSlice, std::to_underlying(startType))};
    assert(error == Error::success);

    error = Error{tx_thread_entry_exit_notify(this, ThreadBase::entryExitNotifyCallback)};
    assert(error == Error::success);
}

Error ThreadBase::resume()
{
    return Error{tx_thread_resume(this)};
}

Error ThreadBase::suspend()
{
    return Error{tx_thread_suspend(this)};
}

Error ThreadBase::restart()
{
    if (auto error = Error{tx_thread_reset(this)}; error != Error::success)
    {
        return error;
    }

    return Error{tx_thread_resume(this)};
}

Error ThreadBase::terminate()
{
    return Error{tx_thread_terminate(this)};
}

Error ThreadBase::abortWait()
{
    return Error{tx_thread_wait_abort(this)};
}

ThreadBase::ID ThreadBase::id() const
{
    return ID(static_cast<const Native::TX_THREAD *>(this));
}

std::string_view ThreadBase::name() const
{
    return tx_thread_name;
}

ThreadBase::State ThreadBase::state() const
{
    return State{tx_thread_state};
}

ThreadBase::UintPair ThreadBase::preemption(const auto newPreempt)
{
    Uint oldPreempt{};
    Error error{tx_thread_preemption_change(this, newPreempt, std::addressof(oldPreempt))};

    return {error, oldPreempt};
}

Uint ThreadBase::preemption() const
{
    return tx_thread_user_preempt_threshold;
}

ThreadBase::UintPair ThreadBase::priority(const auto newPriority)
{
    Uint oldPriority;
    Error error{tx_thread_priority_change(this, newPriority, std::addressof(oldPriority))};

    return {error, oldPriority};
}

Uint ThreadBase::priority() const
{
    return tx_thread_user_priority;
}

ThreadBase::UlongPair ThreadBase::timeSlice(const auto newTimeSlice)
{
    Ulong oldTimeSlice;
    Error error{tx_thread_time_slice_change(this, newTimeSlice, std::addressof(oldTimeSlice))};

    return {error, oldTimeSlice};
}

void ThreadBase::join()
{
    assert(not m_exitSignalPtr);
    BinarySemaphore exitSignal("join");

    {
        Kernel::CriticalSection cs; //do not allow any change in thread state until m_exitSignalPtr is assigned.

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

bool ThreadBase::joinable() const
{
    // wait on itself resource deadlock and wait on finished thread.
    auto threadState{state()};
    return id() != ThisThread::id() and threadState != State::completed and threadState != State::terminated;
}

ThreadBase::StackInfo ThreadBase::stackInfo() const
{
    return StackInfo{.size = tx_thread_stack_size,
                     .used = uintptr_t(tx_thread_stack_end) - uintptr_t(tx_thread_stack_ptr) + 1,
                     .maxUsed = uintptr_t(tx_thread_stack_end) - uintptr_t(tx_thread_stack_highest_ptr) + 1,
                     .maxUsedPercent = (uintptr_t(tx_thread_stack_end) - uintptr_t(tx_thread_stack_highest_ptr) + 1) *
                                       100 / tx_thread_stack_size}; // As a rule of thumb, keep this below 70%
}

void ThreadBase::stackErrorNotifyCallback(Native::TX_THREAD *const threadPtr)
{
    auto &thread{static_cast<ThreadBase &>(*threadPtr)};
    thread.m_stackErrorNotifyCallback(thread);
}

void ThreadBase::entryExitNotifyCallback(auto *const threadPtr, const auto condition)
{
    auto &thread{static_cast<ThreadBase &>(*threadPtr)};
    auto notifyCondition{NotifyCondition{condition}};

    if (thread.m_entryExitNotifyCallback)
    {
        thread.m_entryExitNotifyCallback(thread, notifyCondition);
    }

    if (notifyCondition == NotifyCondition::exit)
    {
        if (thread.m_exitSignalPtr)
        {
            [[maybe_unused]] auto error{thread.m_exitSignalPtr->release()};
            assert(error == Error::success);
        }
    }
}

void ThreadBase::entryFunction(auto thisPtr)
{
    reinterpret_cast<ThreadBase *>(thisPtr)->entryCallback();
}
} // namespace ThreadX

namespace ThreadX::ThisThread
{
ThreadBase::ID id()
{
    return ThreadBase::ID(Native::tx_thread_identify());
}

void yield()
{
    Native::tx_thread_relinquish();
}
} // namespace ThreadX::ThisThread