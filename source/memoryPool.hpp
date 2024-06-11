#pragma once

#include "tickTimer.hpp"
#include "txCommon.hpp"
#include <array>

namespace ThreadX
{
class BytePoolBase : protected Native::TX_BYTE_POOL
{
  public:
    BytePoolBase(const BytePoolBase &) = delete;
    BytePoolBase &operator=(const BytePoolBase &) = delete;

    static constexpr auto minimumPoolSize(std::span<const Ulong> memorySizes);

    Error release(void *memoryPtr);

    template <typename Rep = TickTimer::rep, typename Period = TickTimer::period>
    std::pair<Error, void *> allocate(
        const Ulong memorySizeInBytes, const std::chrono::duration<Rep, Period> &duration = TickTimer::noWait);

    /// Places the highest priority thread suspended for memory on this pool at the front of the suspension list.
    /// All other threads remain in the same FIFO order they were suspended in.
    Error prioritise();
    std::string_view name() const;

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
    const Ulong memorySizeInBytes, const std::chrono::duration<Rep, Period> &duration)
{
    void *memoryPtr;
    Error error{tx_byte_allocate(this, std::addressof(memoryPtr), memorySizeInBytes,
                                 TickTimer::ticks(std::chrono::duration_cast<TickTimer::Duration>(duration)))};
    return {error, memoryPtr};
}

/// byte memory pool from which to allocate the thread stacks and queues.
/// \tparam Size size of byte pool in bytes
template <Ulong Size> class BytePool : public BytePoolBase, std::array<Ulong, Size / sizeOfUlong> // Ulong alignment
{
    static_assert(Size % sizeOfUlong == 0, "Pool size must be a multiple of Ulong size.");

  public:
    ///
    BytePool(const std::string_view name);
};

template <Ulong Size> BytePool<Size>::BytePool(const std::string_view name)
{
    using namespace Native;
    [[maybe_unused]] Error error{tx_byte_pool_create(this, const_cast<char *>(name.data()), this->data(), Size)};
    assert(error == Error::success);
}

class BlockPoolBase : protected Native::TX_BLOCK_POOL
{
  public:
    BlockPoolBase(const BlockPoolBase &) = delete;
    BlockPoolBase &operator=(const BlockPoolBase &) = delete;

    Ulong blockSize() const;

    Error release(void *memoryPtr);

    template <typename Rep = TickTimer::rep, typename Period = TickTimer::period>
    std::pair<Error, void *> allocate(const std::chrono::duration<Rep, Period> &duration = TickTimer::noWait);

    /// Places the highest priority thread suspended for memory on this pool at the front of the suspension list.
    /// All other threads remain in the same FIFO order they were suspended in.
    Error prioritise();
    std::string_view name() const;

  protected:
    BlockPoolBase();
    ///
    ~BlockPoolBase();
};

template <typename Rep, typename Period>
std::pair<Error, void *> BlockPoolBase::allocate(const std::chrono::duration<Rep, Period> &duration)
{
    void *memoryPtr;
    Error error{tx_block_allocate(
        this, std::addressof(memoryPtr), TickTimer::ticks(std::chrono::duration_cast<TickTimer::Duration>(duration)))};
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
    BlockPool(const std::string_view name);
};

template <Ulong Size, Ulong BlockSize> BlockPool<Size, BlockSize>::BlockPool(const std::string_view name)
{
    using namespace Native;
    [[maybe_unused]] Error error{
        tx_block_pool_create(this, const_cast<char *>(name.data()), BlockSize, this->data(), Size)};
    assert(error == Error::success);
}
} // namespace ThreadX
