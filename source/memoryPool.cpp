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

Error BytePoolBase::prioritise()
{
    return Error{tx_byte_pool_prioritize(this)};
}

std::string_view BytePoolBase::name() const
{
    return std::string_view(tx_byte_pool_name);
}

BlockPoolBase::BlockPoolBase() : Native::TX_BLOCK_POOL{}
{
}

BlockPoolBase::~BlockPoolBase()
{
    tx_block_pool_delete(this);
}

Ulong BlockPoolBase::blockSize() const
{
    return tx_block_pool_block_size;
}

Error BlockPoolBase::release(void *memoryPtr)
{
    return Error{Native::tx_block_release(memoryPtr)};
}

Error BlockPoolBase::prioritise()
{
    return Error{tx_block_pool_prioritize(this)};
}

std::string_view BlockPoolBase::name() const
{
    return std::string_view(tx_block_pool_name);
}
} // namespace ThreadX
