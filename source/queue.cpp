#include "queue.hpp"

namespace ThreadX
{
QueueBase::QueueBase(MemoryPoolBase &pool) : Native::TX_QUEUE{}, m_pool{pool}
{
}

QueueBase::~QueueBase()
{
    tx_queue_delete(this);
    m_pool.release(tx_queue_start);
}

Error QueueBase::prioritise()
{
    return Error{tx_queue_prioritize(this)};
}

Error QueueBase::flush()
{
    return Error{tx_queue_flush(this)};
}

std::string_view QueueBase::name() const
{
    return tx_queue_name;
}
} // namespace ThreadX
