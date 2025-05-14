#include "mutex.hpp"
#include <cassert>
#include <string_view>
#include <utility>

namespace ThreadX
{
Mutex::Mutex(const InheritMode inheritMode) : Mutex("mutex", inheritMode)
{
}

Mutex::Mutex(const std::string_view name, const InheritMode inheritMode) : Native::TX_MUTEX{}
{
    using namespace Native;
    [[maybe_unused]] Error error{tx_mutex_create(this, const_cast<char *>(name.data()), std::to_underlying(inheritMode))};
    assert(error == Error::success);
}

Mutex::~Mutex()
{
    [[maybe_unused]] Error error{tx_mutex_delete(this)};
    assert(error == Error::success);
}

auto Mutex::lock() -> Error
{
    return try_lock_for(TickTimer::waitForever);
}

auto Mutex::try_lock() -> Error
{
    return try_lock_for(TickTimer::noWait);
}

auto Mutex::unlock() -> Error
{
    return Error{tx_mutex_put(this)};
}

auto Mutex::name() const -> std::string_view
{
    return std::string_view{tx_mutex_name};
}

auto Mutex::prioritise() -> Error
{
    return Error{tx_mutex_prioritize(this)};
}

auto Mutex::lockingThreadID() const -> uintptr_t
{
    return uintptr_t(tx_mutex_owner);
}
} // namespace ThreadX
