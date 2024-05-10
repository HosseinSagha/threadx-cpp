#include "semaphore.hpp"

namespace ThreadX
{
SemaphoreBase::SemaphoreBase(const std::string_view name, const Ulong initialCount) : Native::TX_SEMAPHORE{}
{
    using namespace Native;
    [[maybe_unused]] Error error{tx_semaphore_create(this, const_cast<char *>(name.data()), initialCount)};
    assert(error == Error::success);
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

} // namespace ThreadX
