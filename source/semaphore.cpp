#include "semaphore.hpp"

namespace ThreadX
{
SemaphoreBase::SemaphoreBase(const Ulong ceiling, const std::string_view name, const Ulong initialCount,
                             const NotifyCallback &releaseNotifyCallback)
    : Native::TX_SEMAPHORE{}, m_ceiling{ceiling}, m_releaseNotifyCallback{releaseNotifyCallback}
{
    assert(initialCount <= ceiling);

    using namespace Native;
    [[maybe_unused]] Error error{tx_semaphore_create(this, const_cast<char *>(name.data()), initialCount)};
    assert(error == Error::success);

    if (m_releaseNotifyCallback)
    {
        error = Error{tx_semaphore_put_notify(this, SemaphoreBase::releaseNotifyCallback)};
        assert(error == Error::success);
    }
}

SemaphoreBase::~SemaphoreBase()
{
    tx_semaphore_delete(this);
}

Error SemaphoreBase::acquire()
{
    return tryAcquireFor(TickTimer::waitForever);
}

Error SemaphoreBase::tryAcquire()
{
    return tryAcquireFor(TickTimer::noWait);
}

Error SemaphoreBase::release(Ulong count)
{
    while (count > 0)
    {
        if (Error error{tx_semaphore_ceiling_put(this, m_ceiling)}; error != Error::success)
        {
            return error;
        }

        --count;
    }

    return Error::success;
}

Error SemaphoreBase::prioritise()
{
    return Error{tx_semaphore_prioritize(this)};
}

std::string_view SemaphoreBase::name() const
{
    return std::string_view(tx_semaphore_name);
}

Ulong SemaphoreBase::count() const
{
    return tx_semaphore_count;
}

void SemaphoreBase::releaseNotifyCallback(auto notifySemaphorePtr)
{
    auto &semaphore{static_cast<SemaphoreBase &>(*notifySemaphorePtr)};
    semaphore.m_releaseNotifyCallback(semaphore);
}
} // namespace ThreadX
