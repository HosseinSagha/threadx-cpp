#include "thread.hpp"
#include <utility>

namespace ThreadX::ThisThread
{
ID id()
{
    return reinterpret_cast<ID>(Native::tx_thread_identify());
}

void yield()
{
    Native::tx_thread_relinquish();
}
} // namespace ThreadX::ThisThread
