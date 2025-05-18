#pragma once

#include "memoryPool.hpp"
#include "txCommon.hpp"
#include <cassert>

namespace ThreadX
{
template <class Pool, typename T = std::byte>
class Allocator final
{
  public:
    using value_type = T;

    Allocator &operator=(const Allocator &) = delete;

    explicit Allocator(Pool &pool);

    [[nodiscard]] auto allocate(std::size_t n) -> T *requires(Pool::isBytePool());

    template <typename Rep, typename Period>
    [[nodiscard]] auto allocate(std::size_t n, const std::chrono::duration<Rep, Period> &duration) -> T *requires(Pool::isBytePool());

    [[nodiscard]] auto allocate(std::size_t n) -> T *requires(not Pool::isBytePool());

    template <typename Rep, typename Period>
    [[nodiscard]] auto allocate(std::size_t n, const std::chrono::duration<Rep, Period> &duration) -> T *requires(not Pool::isBytePool());

    auto deallocate(T *allocationPtr, std::size_t n) -> void
        requires(Pool::isBytePool());

    auto deallocate(T *allocationPtr, std::size_t n) -> void
        requires(not Pool::isBytePool());

  private:
    Pool &m_pool;
};

template <class Pool, typename T>
Allocator<Pool, T>::Allocator(Pool &pool) : m_pool{pool}
{
}

template <class Pool, typename T>
auto Allocator<Pool, T>::allocate(std::size_t n) -> T *requires(Pool::isBytePool())

{ return allocate(n, TickTimer::noWait); }

template <class Pool, typename T>
template <typename Rep, typename Period>
auto Allocator<Pool, T>::allocate(std::size_t n, const std::chrono::duration<Rep, Period> &duration) -> T *requires(Pool::isBytePool()) {
    if (n > std::numeric_limits<Ulong>::max() / sizeof(T))
    {
        return nullptr;
    }

    std::byte * allocationPtr{};

    [[maybe_unused]] Error error{tx_byte_allocate(std::addressof(m_pool), reinterpret_cast<void **>(std::addressof(allocationPtr)),
                                                  static_cast<Ulong>(n * sizeof(T)), TickTimer::ticks(duration))};
    if (error != Error::success)
    {
        return nullptr;
    }

    return reinterpret_cast<T *>(allocationPtr);
}

template <class Pool, typename T>
auto Allocator<Pool, T>::allocate(std::size_t n) -> T *requires(not Pool::isBytePool())

{ return allocate(n, TickTimer::noWait); }

template <class Pool, typename T>
template <typename Rep, typename Period>
auto Allocator<Pool, T>::allocate(std::size_t n, const std::chrono::duration<Rep, Period> &duration) -> T *requires(not Pool::isBytePool()) {
    if (n > std::numeric_limits<Ulong>::max() / sizeof(T) or static_cast<Ulong>(n * sizeof(T)) > m_pool.blockSize())
    {
        return nullptr;
    }

    std::byte * allocationPtr{};

    [[maybe_unused]] Error error{
        tx_block_allocate(std::addressof(m_pool), reinterpret_cast<void **>(std::addressof(allocationPtr)), TickTimer::ticks(duration))};
    if (error != Error::success)
    {
        return nullptr;
    }

    return reinterpret_cast<T *>(allocationPtr);
}

template <class Pool, typename T>
auto Allocator<Pool, T>::deallocate(T *allocationPtr, [[maybe_unused]] std::size_t n) -> void
    requires(Pool::isBytePool())
{
    [[maybe_unused]] Error error{Native::tx_byte_release(allocationPtr)};
    assert(error == Error::success);
}

template <class Pool, typename T>
auto Allocator<Pool, T>::deallocate(T *allocationPtr, [[maybe_unused]] std::size_t n) -> void
    requires(not Pool::isBytePool())
{
    [[maybe_unused]] Error error{Native::tx_block_release(allocationPtr)};
    assert(error == Error::success);
}

template <class T, class U>
auto operator==(const Allocator<T> &lhs, const Allocator<U> &rhs) -> bool
{
    return std::addressof(lhs.m_pool) == std::addressof(rhs.m_pool) and T::value_type == U::value_type;
}

// TODO: Remove this when C++26 definition is available
template <class Alloc>
concept SimpleAllocator = requires(Alloc alloc, std::size_t n) {
    { *alloc.allocate(n) };
    { alloc.deallocate(alloc.allocate(n), n) };
} && std::copy_constructible<Alloc> && std::equality_comparable<Alloc>;

static_assert(SimpleAllocator<Allocator<BytePool<4>>>);
static_assert(SimpleAllocator<Allocator<BlockPool<2, 2>>>);
} // namespace ThreadX
