#include "thread.hpp"
#include "kernel.hpp"

namespace ThreadX::ThisThread
{
uintptr_t id()
{
    return uintptr_t(Native::tx_thread_identify());
}

void yield()
{
    Native::tx_thread_relinquish();
}

Error sleepFor(const TickTimer::Duration &duration)
{
    return Error{Native::tx_thread_sleep(TickTimer::ticks(duration))};
}

Error sleepUntil(const TickTimer::TimePoint &timePoint)
{
    return sleepFor(timePoint - TickTimer::now());
}
} // namespace ThreadX::ThisThread

namespace ThreadX
{
Thread::Thread(BytePoolBase &pool, const Ulong stackSize, const NotifyCallback &entryExitNotifyCallback,
               const Uint priority, const Uint preamptionThresh, const Ulong timeSlice, const StartType startType)
    : Native::TX_THREAD{}, m_pool{pool}, m_entryExitNotifyCallback{entryExitNotifyCallback}
{
    auto [error, stackPtr] = pool.allocate(stackSize);
    assert(error == Error::success);

    m_stackPtr = stackPtr;

    using namespace Native;
    error = Error{tx_thread_create(
        this, const_cast<char *>("thread"), entryFunction, reinterpret_cast<Ulong>(this), m_stackPtr, stackSize,
        priority, preamptionThresh, timeSlice, static_cast<Uint>(startType))};
    assert(error == Error::success);

    error = Error{tx_thread_entry_exit_notify(this, Thread::entryExitNotifyCallback)};
    assert(error == Error::success);
}

Thread::Thread(BlockPoolBase &pool, const NotifyCallback &entryExitNotifyCallback, const Uint priority,
               const Uint preamptionThresh, const Ulong timeSlice, const StartType startType)
    : Native::TX_THREAD{}, m_pool{pool}, m_entryExitNotifyCallback{entryExitNotifyCallback}
{
    auto [error, stackPtr] = pool.allocate();
    assert(error == Error::success);

    m_stackPtr = stackPtr;

    using namespace Native;
    error = Error{tx_thread_create(
        this, const_cast<char *>("thread"), entryFunction, reinterpret_cast<Ulong>(this), m_stackPtr, pool.blockSize(),
        priority, preamptionThresh, timeSlice, static_cast<Uint>(startType))};
    assert(error == Error::success);

    error = Error{tx_thread_entry_exit_notify(this, Thread::entryExitNotifyCallback)};
    assert(error == Error::success);
}

Thread::~Thread()
{
    terminate();
    tx_thread_delete(this);
    m_pool.release(m_stackPtr);
}

Error Thread::registerStackErrorNotifyCallback(const ErrorCallback &stackErrorNotifyCallback)
{
    if (Error error{
            tx_thread_stack_error_notify(stackErrorNotifyCallback ? Thread::stackErrorNotifyCallback : nullptr)};
        error != Error::success)
    {
        return error;
    }

    m_stackErrorNotifyCallback = stackErrorNotifyCallback;
    return Error::success;
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
    Error error{};

    if (error = Error{tx_thread_reset(this)}; error != Error::success)
    {
        return error;
    }

    // lock semaphore in case join is not called, for the next thread run
    if (error = m_exitSignal.tryAcquire(); error != Error::success)
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

uintptr_t Thread::id()
{
    return uintptr_t(static_cast<Native::TX_THREAD *>(this));
}

ThreadState Thread::state() const
{
    return ThreadState{tx_thread_state};
}

Thread::ReturnTuple Thread::changePreemption(const auto newPreempt)
{
    Uint oldPreempt{};
    Error error{tx_thread_preemption_change(this, newPreempt, std::addressof(oldPreempt))};

    return {error, oldPreempt};
}

Thread::ReturnTuple Thread::changePriority(const auto newPriority)
{
    Uint oldPriority;
    Error error{tx_thread_priority_change(this, newPriority, std::addressof(oldPriority))};

    return {error, oldPriority};
}

Thread::ReturnTuple Thread::changeTimeSlice(const auto newTimeSlice)
{
    Ulong oldTimeSlice;
    Error error{tx_thread_time_slice_change(this, newTimeSlice, std::addressof(oldTimeSlice))};

    return {error, oldTimeSlice};
}

void Thread::join()
{
    LockGuard lockGuard(m_joinMutex); // if multiple threads wait for this thread to join

    if (not joinable()) // only first call will progress. thread becomes unjoinable by the time entryExitNotifyCallback() is called.
    {
        return;
    }

    m_exitSignal.acquire(); // wait for release by exit notify callback
}

bool Thread::joinable()
{
    // resource deadlock, wait on finished thread and being waited by multiple threads
    auto threadState{state()};
    return id() != ThisThread::id() and threadState != ThreadState::completed and
           threadState != ThreadState::terminated;
}

void Thread::entryFunction(auto thisPtr)
{
    reinterpret_cast<Thread *>(thisPtr)->entryCallback();
}

void Thread::stackErrorNotifyCallback(Native::TX_THREAD *threadPtr)
{
    auto &thread{static_cast<Thread &>(*threadPtr)};
    thread.m_stackErrorNotifyCallback(thread);
}

void Thread::entryExitNotifyCallback(auto threadPtr, auto condition)
{
    auto &thread{static_cast<Thread &>(*threadPtr)};
    auto notifyCondition{NotifyCondition{condition}};

    if (thread.m_entryExitNotifyCallback)
    {
        thread.m_entryExitNotifyCallback(thread, notifyCondition);
    }

    if (notifyCondition == NotifyCondition::exit)
    {
        thread.m_exitSignal.release();
    }
}
} // namespace ThreadX