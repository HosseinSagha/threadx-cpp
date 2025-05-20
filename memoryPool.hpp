#pragma once

#include "tickTimer.hpp"
#include "txCommon.hpp"
#include <array>
#include <cassert>
#include <string_view>

namespace ThreadX
{
/// byte memory pool from which to allocate the thread stacks and queues.
/// \tparam Size size of byte pool in bytes
template <Ulong Size>
class BytePool final : Native::TX_BYTE_POOL
{
    static_assert(Size % wordSize == 0, "Pool size must be a multiple of word size.");

  public:
    template <class Pool, typename T>
    friend class Allocator;

    [[nodiscard]] static consteval auto isBytePool() -> bool;

    explicit BytePool(const std::string_view name);
    ~BytePool();

    /// Places the highest priority thread suspended for memory on this pool at the front of the suspension list.
    /// All other threads remain in the same FIFO order they were suspended in.
    auto prioritise() -> Error;

    [[nodiscard]] auto name() const -> std::string_view;

  private:
    std::array<Ulong, Size / wordSize> m_pool{}; // Ulong alignment
};

template <Ulong Size>
consteval auto BytePool<Size>::isBytePool() -> bool
{
    return true;
}

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
auto BytePool<Size>::prioritise() -> Error
{
    return Error{tx_byte_pool_prioritize(this)};
}

template <Ulong Size>
auto BytePool<Size>::name() const -> std::string_view
{
    return std::string_view{tx_byte_pool_name};
}

template <Ulong Blocks, Ulong BlockSize>
class BlockPool final : Native::TX_BLOCK_POOL
{
  public:
    [[nodiscard]] static consteval auto blockSize() -> Ulong;
    [[nodiscard]] static consteval auto isBytePool() -> bool;

    /// block memory pool from which to allocate the thread stacks and queues.
    /// total blocks = (total bytes) / (block size + sizeof(uintptr_t))
    explicit BlockPool(const std::string_view name);
    ~BlockPool();

    /// Places the highest priority thread suspended for memory on this pool at the front of the suspension list.
    /// All other threads remain in the same FIFO order they were suspended in.
    auto prioritise() -> Error;

    [[nodiscard]] auto name() const -> std::string_view;

  private:
    static constexpr Ulong Size{Blocks * (BlockSize + sizeof(uintptr_t))};
    static_assert(Size % wordSize == 0, "Pool size must be a multiple of word size.");

    std::array<Ulong, Size / wordSize> m_pool{}; // Ulong alignment
};

template <Ulong Blocks, Ulong BlockSize>
consteval auto BlockPool<Blocks, BlockSize>::blockSize() -> Ulong
{
    return BlockSize;
}

template <Ulong Blocks, Ulong BlockSize>
consteval auto BlockPool<Blocks, BlockSize>::isBytePool() -> bool
{
    return false;
}

template <Ulong Blocks, Ulong BlockSize>
BlockPool<Blocks, BlockSize>::BlockPool(const std::string_view name) : Native::TX_BLOCK_POOL{}
{
    using namespace Native;
    [[maybe_unused]] Error error{tx_block_pool_create(this, const_cast<char *>(name.data()), BlockSize, m_pool.data(), Size)};
    assert(error == Error::success);
}

template <Ulong Blocks, Ulong BlockSize>
BlockPool<Blocks, BlockSize>::~BlockPool()
{
    [[maybe_unused]] Error error{tx_block_pool_delete(this)};
    assert(error == Error::success);
}

template <Ulong Blocks, Ulong BlockSize>
auto BlockPool<Blocks, BlockSize>::prioritise() -> Error
{
    return Error{tx_block_pool_prioritize(this)};
}

template <Ulong Blocks, Ulong BlockSize>
auto BlockPool<Blocks, BlockSize>::name() const -> std::string_view
{
    return std::string_view{tx_block_pool_name};
}

constexpr auto minimumBytePoolSize(std::span<const Ulong> allocationSizes) -> Ulong
{
    Ulong poolSize{2 * sizeof(uintptr_t)};
    for (auto memSize : allocationSizes)
    {
        poolSize += (memSize + 2 * sizeof(uintptr_t));
    }

    return poolSize;
}
} // namespace ThreadX
