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
} // namespace ThreadX::ThisThread
