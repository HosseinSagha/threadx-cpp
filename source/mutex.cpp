#include "mutex.hpp"
#include <utility>

namespace ThreadX
{
Mutex::Mutex(const InheritMode inheritMode) : Mutex("mutex", inheritMode)
{
}

Mutex::Mutex(const std::string_view name, const InheritMode inheritMode) : Native::TX_MUTEX{}
{
    using namespace Native;
    [[maybe_unused]] Error error{
        tx_mutex_create(this, const_cast<char *>(name.data()), std::to_underlying(inheritMode))};
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

Error Mutex::unlock()
{
    return Error{tx_mutex_put(this)};
}

std::string_view Mutex::name() const
{
    return tx_mutex_name;
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
