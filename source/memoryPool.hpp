#pragma once

#include "tickTimer.hpp"
#include "txCommon.hpp"
#include <array>

namespace ThreadX
{
class MemoryPoolBase
{
  public:
    virtual Error release(void *memoryPtr) = 0;

  protected:
    virtual ~MemoryPoolBase() = default;
};

class BytePoolBase : public MemoryPoolBase, protected Native::TX_BYTE_POOL
{
  public:
    BytePoolBase(const BytePoolBase &) = delete;
    BytePoolBase &operator=(const BytePoolBase &) = delete;

    static constexpr auto minimumPoolSize(std::span<const Ulong> memorySizes);

    Error release(void *memoryPtr) final;

    template <typename Rep = TickTimer::rep, typename Period = TickTimer::period>
    std::pair<Error, void *> allocate(
        const Ulong memorySizeInBytes, const std::chrono::duration<Rep, Period> &waitDuration = TickTimer::noWait);

    /// Places the highest priority thread suspended for memory on this pool at the front of the suspension list.
    /// All other threads remain in the same FIFO order they were suspended in.
    Error prioritise();

  protected:
    BytePoolBase();
    ///
    ~BytePoolBase();
};

constexpr auto BytePoolBase::minimumPoolSize(std::span<const Ulong> memorySizes)
{
    Ulong poolSize{2 * sizeof(uintptr_t)};
    for (auto memSize : memorySizes)
    {
        poolSize += (memSize + 2 * sizeof(uintptr_t));
    }

    return poolSize;
}

template <typename Rep, typename Period>
std::pair<Error, void *> BytePoolBase::allocate(
    const Ulong memorySizeInBytes, const std::chrono::duration<Rep, Period> &waitDuration)
{
    void *memoryPtr;
    Error error{tx_byte_allocate(this, std::addressof(memoryPtr), memorySizeInBytes,
                                 TickTimer::ticks(std::chrono::duration_cast<TickTimer::Duration>(waitDuration)))};
    return {error, memoryPtr};
}

/// byte memory pool from which to allocate the thread stacks and queues.
/// \tparam Size size of byte pool in bytes
template <Ulong Size> class BytePool : public BytePoolBase, std::array<Ulong, Size / sizeOfUlong> // Ulong alignment
{
    static_assert(Size % sizeOfUlong == 0, "Pool size must be a multiple of Ulong size.");

  public:
    ///
    BytePool();
};

template <Ulong Size> BytePool<Size>::BytePool()
{
    using namespace Native;
    [[maybe_unused]] Error error{tx_byte_pool_create(this, const_cast<char *>("byte pool"), this->data(), Size)};
    assert(error == Error::success);
}

class BlockPoolBase : public MemoryPoolBase, protected Native::TX_BLOCK_POOL
{
  public:
    constexpr Ulong blockSize();

    BlockPoolBase(const BlockPoolBase &) = delete;
    BlockPoolBase &operator=(const BlockPoolBase &) = delete;

    Error release(void *memoryPtr) final;

    template <typename Rep = TickTimer::rep, typename Period = TickTimer::period>
    std::pair<Error, void *> allocate(const std::chrono::duration<Rep, Period> &waitDuration = TickTimer::noWait);

    /// Places the highest priority thread suspended for memory on this pool at the front of the suspension list.
    /// All other threads remain in the same FIFO order they were suspended in.
    Error prioritise();

  protected:
    BlockPoolBase();
    ///
    ~BlockPoolBase();
};

constexpr Ulong BlockPoolBase::blockSize()
{
    return tx_block_pool_block_size;
}

template <typename Rep, typename Period>
std::pair<Error, void *> BlockPoolBase::allocate(const std::chrono::duration<Rep, Period> &waitDuration)
{
    void *memoryPtr;
    Error error{tx_block_allocate(this, std::addressof(memoryPtr),
                                  TickTimer::ticks(std::chrono::duration_cast<TickTimer::Duration>(waitDuration)))};
    return {error, memoryPtr};
}

template <Ulong Size, Ulong BlockSize>
class BlockPool : public BlockPoolBase, std::array<Ulong, Size / sizeOfUlong> // Ulong alignment
{
    static_assert(Size % sizeOfUlong == 0, "Pool size must be a multiple of Ulong size.");
    static_assert(Size % (BlockSize + sizeof(void *)) == 0);

  public:
    /// block memory pool from which to allocate the thread stacks and queues.
    /// total blocks = (total bytes) / (block size + sizeof(void *))
    BlockPool();
};

template <Ulong Size, Ulong BlockSize> BlockPool<Size, BlockSize>::BlockPool()
{
    using namespace Native;
    [[maybe_unused]] Error error{
        tx_block_pool_create(this, const_cast<char *>("block pool"), BlockSize, this->data(), Size)};
    assert(error == Error::success);
}
} // namespace ThreadX
