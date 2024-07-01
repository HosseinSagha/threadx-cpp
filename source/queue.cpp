#include "queue.hpp"

namespace ThreadX
{
QueueBaseBase::QueueBaseBase() : Native::TX_QUEUE{}
{
}

QueueBaseBase::~QueueBaseBase()
{
    tx_queue_delete(this);
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
