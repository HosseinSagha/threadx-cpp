#include "thisThread.hpp"
#include <utility>

namespace ThreadX::ThisThread
{
auto id() -> ID
{
    return reinterpret_cast<ID>(Native::tx_thread_identify());
}

auto yield() -> void
{
    Native::tx_thread_relinquish();
}

auto terminate() -> Error
{
    return Error{tx_thread_terminate(Native::tx_thread_identify())};
}

auto suspend() -> Error
{
    return Error{tx_thread_suspend(Native::tx_thread_identify())};
}
} // namespace ThreadX::ThisThread
