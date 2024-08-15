#include "queue.hpp"

namespace ThreadX
{
QueueBaseBase::QueueBaseBase() : Native::TX_QUEUE{}
{
}

QueueBaseBase::~QueueBaseBase()
{
    [[maybe_unused]] Error error{tx_queue_delete(this)};
    assert(error == Error::success);
}

Error QueueBaseBase::prioritise()
{
    return Error{tx_queue_prioritize(this)};
}

Error QueueBaseBase::flush()
{
    return Error{tx_queue_flush(this)};
}

std::string_view QueueBaseBase::name() const
{
    return tx_queue_name;
}
} // namespace ThreadX
