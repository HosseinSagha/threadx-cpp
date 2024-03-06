#include "semaphore.hpp"

namespace ThreadX
{
Error CountingSemaphore::releaseImpl()
{
    return Error{tx_semaphore_put(this)};
}
} // namespace ThreadX
