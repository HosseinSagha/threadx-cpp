#include "mutex.hpp"

namespace ThreadX
{
Mutex::Mutex(const InheritMode inheritMode) : Native::TX_MUTEX{}
{
    using namespace Native;
    [[maybe_unused]] Error error{tx_mutex_create(this, const_cast<char *>("mutex"), static_cast<Uint>(inheritMode))};
    assert(error == Error::success);
}

Mutex::~Mutex()
{
    [[maybe_unused]] Error error{tx_mutex_delete(this)};
    assert(error == Error::success);
}

Error Mutex::lock()
{
    return try_lock_for(TickTimer::waitForever);
}

Error Mutex::try_lock()
{
    return try_lock_for(TickTimer::noWait);
}

Error Mutex::try_lock_until(const TickTimer::TimePoint &timePoint)
{
    return try_lock_for(timePoint - TickTimer::now());
}

Error Mutex::try_lock_for(const TickTimer::Duration &duration)
{
    return Error{tx_mutex_get(this, TickTimer::ticks(duration))};
}

Error Mutex::unlock()
{
    return Error{tx_mutex_put(this)};
}

Error Mutex::prioritise()
{
    return Error{tx_mutex_prioritize(this)};
}

uintptr_t Mutex::lockingThreadID() const
{
    return uintptr_t(tx_mutex_owner);
}
} // namespace ThreadX
