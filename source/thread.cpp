#include "thread.hpp"
#include "kernel.hpp"
#include <utility>

namespace ThreadX::ThisThread
{
Thread::ID id()
{
    return Thread::ID(Native::tx_thread_identify());
}

void yield()
{
    Native::tx_thread_relinquish();
}

Error sleepFor(const TickTimer::Duration &duration)
{
    return Error{Native::tx_thread_sleep(TickTimer::ticks(duration))};
}

Error sleepUntil(const TickTimer::TimePoint &time)
{
    return sleepFor(time - TickTimer::now());
}
} // namespace ThreadX::ThisThread

namespace ThreadX
{
Thread::Thread(
    const std::string_view name, BytePoolBase &pool, const Ulong stackSize, const NotifyCallback &entryExitNotifyCallback,
    const Uint priority, const Uint preamptionThresh, const Ulong timeSlice, const StartType startType)
    : Native::TX_THREAD{}, m_pool{pool}, m_entryExitNotifyCallback{entryExitNotifyCallback}
{
    auto [error, stackPtr] = pool.allocate(stackSize);
    assert(error == Error::success);

    using namespace Native;
    error = Error{tx_thread_create(
        this, const_cast<char *>(name.data()), entryFunction, reinterpret_cast<Ulong>(this), stackPtr, stackSize,
        priority, preamptionThresh, timeSlice, std::to_underlying(startType))};
    assert(error == Error::success);

    error = Error{tx_thread_entry_exit_notify(this, Thread::entryExitNotifyCallback)};
    assert(error == Error::success);
}

Thread::Thread(const std::string_view name, BlockPoolBase &pool, const NotifyCallback &entryExitNotifyCallback,
               const Uint priority, const Uint preamptionThresh, const Ulong timeSlice, const StartType startType)
    : Native::TX_THREAD{}, m_pool{pool}, m_entryExitNotifyCallback{entryExitNotifyCallback}
{
    auto [error, stackPtr] = pool.allocate();
    assert(error == Error::success);

    using namespace Native;
    error = Error{tx_thread_create(
        this, const_cast<char *>(name.data()), entryFunction, reinterpret_cast<Ulong>(this), stackPtr, pool.blockSize(),
        priority, preamptionThresh, timeSlice, std::to_underlying(startType))};
    assert(error == Error::success);

    error = Error{tx_thread_entry_exit_notify(this, Thread::entryExitNotifyCallback)};
    assert(error == Error::success);
}

Thread::~Thread()
{
    terminate();
    tx_thread_delete(this);
    m_pool.release(tx_thread_stack_start);
}

Error Thread::registerStackErrorNotifyCallback(const ErrorCallback &stackErrorNotifyCallback)
{
    Error error{tx_thread_stack_error_notify(stackErrorNotifyCallback ? Thread::stackErrorNotifyCallback : nullptr)};
    if (error == Error::success)
    {
        m_stackErrorNotifyCallback = stackErrorNotifyCallback;
    }

    return error;
}

Error Thread::resume()
{
    return Error{tx_thread_resume(this)};
}

Error Thread::suspend()
{
    return Error{tx_thread_suspend(this)};
}

Error Thread::restart()
{
    if (auto error = Error{tx_thread_reset(this)}; error != Error::success)
    {
        return error;
    }

    return Error{tx_thread_resume(this)};
}

Error Thread::terminate()
{
    return Error{tx_thread_terminate(this)};
}

Error Thread::waitAbort()
{
    return Error{tx_thread_wait_abort(this)};
}

Thread::ID Thread::id()
{
    return ID(static_cast<Native::TX_THREAD *>(this));
}

std::string_view Thread::name()
{
    return tx_thread_name;
}

ThreadState Thread::state() const
{
    return ThreadState{tx_thread_state};
}

Thread::ReturnPair Thread::changePreemption(const auto newPreempt)
{
    Uint oldPreempt{};
    Error error{tx_thread_preemption_change(this, newPreempt, std::addressof(oldPreempt))};

    return {error, oldPreempt};
}

Thread::ReturnPair Thread::changePriority(const auto newPriority)
{
    Uint oldPriority;
    Error error{tx_thread_priority_change(this, newPriority, std::addressof(oldPriority))};

    return {error, oldPriority};
}

Thread::ReturnPair Thread::changeTimeSlice(const auto newTimeSlice)
{
    Ulong oldTimeSlice;
    Error error{tx_thread_time_slice_change(this, newTimeSlice, std::addressof(oldTimeSlice))};

    return {error, oldTimeSlice};
}

void Thread::join()
{
    assert(not m_exitSignalPtr);
    Kernel::CriticalSection::lock(); //do not allow any change in thread state until m_exitSignalPtr is assigned.

    if (not joinable()) // Thread becomes unjoinable just before entryExitNotifyCallback() is called.
    {
        Kernel::CriticalSection::unlock();
        return;
    }

    BinarySemaphore exitSignal("Join");
    m_exitSignalPtr = std::addressof(exitSignal);

    Kernel::CriticalSection::unlock();

    [[maybe_unused]] auto error{exitSignal.acquire()}; // wait for release by exit notify callback
    assert(error == Error::success or error == Error::waitAborted);

    m_exitSignalPtr = nullptr;
}

bool Thread::joinable()
{
    // wait on itself resource deadlock and wait on finished thread.
    auto threadState{state()};
    return id() != ThisThread::id() and threadState != ThreadState::completed and
           threadState != ThreadState::terminated;
}

Thread::StackInfo Thread::stackInfo()
{
    return StackInfo{.size = tx_thread_stack_size,
                     .used = uintptr_t(tx_thread_stack_end) - uintptr_t(tx_thread_stack_ptr) + 1,
                     .maxUsed = uintptr_t(tx_thread_stack_end) - uintptr_t(tx_thread_stack_highest_ptr) + 1,
                     .maxUsedPercent = (uintptr_t(tx_thread_stack_end) - uintptr_t(tx_thread_stack_highest_ptr) + 1) *
                                       100 / tx_thread_stack_size}; // As a rule of thumb, keep this below 70%
}

void Thread::entryFunction(auto thisPtr)
{
    reinterpret_cast<Thread *>(thisPtr)->entryCallback();
}

void Thread::entryExitNotifyCallback(auto *const threadPtr, const auto condition)
{
    auto &thread{static_cast<Thread &>(*threadPtr)};
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

void Thread::stackErrorNotifyCallback(Native::TX_THREAD *const threadPtr)
{
    auto &thread{static_cast<Thread &>(*threadPtr)};
    thread.m_stackErrorNotifyCallback(thread);
}
} // namespace ThreadX