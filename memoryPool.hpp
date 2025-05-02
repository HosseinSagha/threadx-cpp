#pragma once

#include "tickTimer.hpp"
#include "txCommon.hpp"
#include <array>
#include <cassert>
#include <string_view>

namespace ThreadX
{
class BytePoolBase
{
  public:
    BytePoolBase(const BytePoolBase &) = delete;
    BytePoolBase &operator=(const BytePoolBase &) = delete;

  protected:
    explicit BytePoolBase() = default;
};

/// byte memory pool from which to allocate the thread stacks and queues.
/// \tparam Size size of byte pool in bytes
template <Ulong Size>
class BytePool final : Native::TX_BYTE_POOL, BytePoolBase
{
    static_assert(Size % wordSize == 0, "Pool size must be a multiple of word size.");

  public:
    template <class Pool>
    friend class Allocation;

    explicit BytePool(const std::string_view name);
    ~BytePool();

    /// Places the highest priority thread suspended for memory on this pool at the front of the suspension list.
    /// All other threads remain in the same FIFO order they were suspended in.
    auto prioritise();
    auto name() const;

  private:
    std::array<Ulong, Size / wordSize> m_pool{}; // Ulong alignment
};

template <Ulong Size>
BytePool<Size>::BytePool(const std::string_view name) : Native::TX_BYTE_POOL{}
{
    using namespace Native;
    [[maybe_unused]] Error error{tx_byte_pool_create(this, const_cast<char *>(name.data()), m_pool.data(), Size)};
    assert(error == Error::success);
}

template <Ulong Size>
BytePool<Size>::~BytePool()
{
    [[maybe_unused]] Error error{tx_byte_pool_delete(this)};
    assert(error == Error::success);
}

template <Ulong Size>
auto BytePool<Size>::prioritise()
{
    return Error{tx_byte_pool_prioritize(this)};
}

template <Ulong Size>
auto BytePool<Size>::name() const
{
    return std::string_view{tx_byte_pool_name};
}

class BlockPoolBase
{
  public:
    BlockPoolBase(const BlockPoolBase &) = delete;
    BlockPoolBase &operator=(const BlockPoolBase &) = delete;

  protected:
    explicit BlockPoolBase() = default;
};

template <Ulong Size, Ulong BlockSize>
class BlockPool final : Native::TX_BLOCK_POOL, BlockPoolBase
{
    static_assert(Size % wordSize == 0, "Pool size must be a multiple of word size.");
    static_assert(Size % (BlockSize + sizeof(std::byte *)) == 0);

  public:
    template <class Pool>
    friend class Allocation;

    /// block memory pool from which to allocate the thread stacks and queues.
    /// total blocks = (total bytes) / (block size + sizeof(std::byte *))
    explicit BlockPool(const std::string_view name);
    ~BlockPool();

    auto blockSize() const;

    /// Places the highest priority thread suspended for memory on this pool at the front of the suspension list.
    /// All other threads remain in the same FIFO order they were suspended in.
    auto prioritise();
    auto name() const;

  private:
    std::array<Ulong, Size / wordSize> m_pool{}; // Ulong alignment
};

template <Ulong Size, Ulong BlockSize>
BlockPool<Size, BlockSize>::BlockPool(const std::string_view name) : Native::TX_BLOCK_POOL{}
{
    using namespace Native;
    [[maybe_unused]] Error error{tx_block_pool_create(this, const_cast<char *>(name.data()), BlockSize, m_pool.data(), Size)};
    assert(error == Error::success);
}

template <Ulong Size, Ulong BlockSize>
BlockPool<Size, BlockSize>::~BlockPool()
{
    [[maybe_unused]] Error error{tx_block_pool_delete(this)};
    assert(error == Error::success);
}

template <Ulong Size, Ulong BlockSize>
auto BlockPool<Size, BlockSize>::blockSize() const
{
    return tx_block_pool_block_size;
}

template <Ulong Size, Ulong BlockSize>
auto BlockPool<Size, BlockSize>::prioritise()
{
    return Error{tx_block_pool_prioritize(this)};
}

template <Ulong Size, Ulong BlockSize>
auto BlockPool<Size, BlockSize>::name() const
{
    return std::string_view{tx_block_pool_name};
}

template <class Pool>
class Allocation final
{
  public:
    Allocation(const Allocation &) = delete;
    Allocation &operator=(const Allocation &) = delete;

    template <typename Rep = TickTimer::rep, typename Period = TickTimer::period>
    Allocation(Pool &pool, const Ulong memorySizeInBytes, const std::chrono::duration<Rep, Period> &duration = TickTimer::noWait)
        requires(std::is_base_of_v<BytePoolBase, Pool>);

    template <typename Rep = TickTimer::rep, typename Period = TickTimer::period>
    Allocation(Pool &pool, const std::chrono::duration<Rep, Period> &duration = TickTimer::noWait)
        requires(std::is_base_of_v<BlockPoolBase, Pool>);

    auto get();

    ~Allocation()
        requires(std::is_base_of_v<BytePoolBase, Pool>);

    ~Allocation()
        requires(std::is_base_of_v<BlockPoolBase, Pool>);

  private:
    std::byte *memoryPtr{};
};

template <class Pool>
template <typename Rep, typename Period>
Allocation<Pool>::Allocation(Pool &pool, const Ulong memorySizeInBytes, const std::chrono::duration<Rep, Period> &duration)
    requires(std::is_base_of_v<BytePoolBase, Pool>)
{
    [[maybe_unused]] Error error{
        tx_byte_allocate(std::addressof(pool), reinterpret_cast<void **>(std::addressof(memoryPtr)), memorySizeInBytes, TickTimer::ticks(duration))};
    assert(error == Error::success);
}

template <class Pool>
template <typename Rep, typename Period>
Allocation<Pool>::Allocation(Pool &pool, const std::chrono::duration<Rep, Period> &duration)
    requires(std::is_base_of_v<BlockPoolBase, Pool>)
{
    [[maybe_unused]] Error error{tx_block_allocate(std::addressof(pool), reinterpret_cast<void **>(std::addressof(memoryPtr)), TickTimer::ticks(duration))};
    assert(error == Error::success);
}

template <class Pool>
auto Allocation<Pool>::get()
{
    return memoryPtr;
}

template <class Pool>
Allocation<Pool>::~Allocation()
    requires(std::is_base_of_v<BytePoolBase, Pool>)
{
    [[maybe_unused]] Error error{Native::tx_byte_release(memoryPtr)};
    assert(error == Error::success);
}

template <class Pool>
Allocation<Pool>::~Allocation()
    requires(std::is_base_of_v<BlockPoolBase, Pool>)
{
    [[maybe_unused]] Error error{Native::tx_block_release(memoryPtr)};
    assert(error == Error::success);
}

constexpr auto minimumPoolSize(std::span<const Ulong> memorySizes)
{
    Ulong poolSize{2 * sizeof(uintptr_t)};
    for (auto memSize : memorySizes)
    {
        poolSize += (memSize + 2 * sizeof(uintptr_t));
    }

    return poolSize;
}
} // namespace ThreadX
