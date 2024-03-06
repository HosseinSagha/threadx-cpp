#include "queue.hpp"

namespace ThreadX
{
QueueBase::QueueBase(MemoryPoolBase &pool) : Native::TX_QUEUE{}, m_pool{pool}
{
}

QueueBase::~QueueBase()
{
    tx_queue_delete(this);
    m_pool.release(m_queueStartPtr);
}

Error QueueBase::prioritise()
{
    return Error{tx_queue_prioritize(this)};
}

Error QueueBase::flush()
{
    return Error{tx_queue_flush(this)};
}
} // namespace ThreadX
