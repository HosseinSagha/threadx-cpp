#include "memoryPool.hpp"

namespace ThreadX
{
BytePoolBase::BytePoolBase() : Native::TX_BYTE_POOL{}
{
}

BytePoolBase::~BytePoolBase()
{
    tx_byte_pool_delete(this);
}

Error BytePoolBase::release(void *memoryPtr)
{
    return Error{Native::tx_byte_release(memoryPtr)};
}

std::pair<Error, void *> BytePoolBase::allocate(const Ulong memorySizeInBytes, const TickTimer::Duration &waitDuration)
{
    void *memoryPtr;
    Error error{tx_byte_allocate(this, std::addressof(memoryPtr), memorySizeInBytes, TickTimer::ticks(waitDuration))};
    return {error, memoryPtr};
}

Error BytePoolBase::prioritise()
{
    return Error{tx_byte_pool_prioritize(this)};
}

BlockPoolBase::BlockPoolBase()
{
}

BlockPoolBase::~BlockPoolBase()
{
    tx_block_pool_delete(this);
}

Error BlockPoolBase::release(void *memoryPtr)
{
    return Error{Native::tx_block_release(memoryPtr)};
}

std::pair<Error, void *> BlockPoolBase::allocate(const TickTimer::Duration &waitDuration)
{
    void *memoryPtr;
    Error error{tx_block_allocate(this, std::addressof(memoryPtr), TickTimer::ticks(waitDuration))};
    return {error, memoryPtr};
}

Error BlockPoolBase::prioritise()
{
    return Error{tx_block_pool_prioritize(this)};
}
} // namespace ThreadX
