#include "thisThread.hpp"

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

auto name() -> std::string_view
{
    return std::string_view{Native::tx_thread_identify()->tx_thread_name};
}
} // namespace ThreadX::ThisThread
