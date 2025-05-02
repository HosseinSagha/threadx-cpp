#pragma once

#include "memoryPool.hpp"
#include "tickTimer.hpp"
#include "txCommon.hpp"
#include <cassert>
#include <chrono>
#include <expected>
#include <functional>
#include <string_view>

namespace ThreadX
{
template <typename Msg, class Pool>
class Queue final : Native::TX_QUEUE
{
  public:
    /// external Notifycallback type
    using NotifyCallback = std::function<void(Queue &)>;

    Queue(const Queue &) = delete;
    Queue &operator=(const Queue &) = delete;

    static constexpr size_t messageSize();

    ///
    /// \param pool byte pool to allocate queue in.
    /// \param queueSizeInNumOfMessages max num of messages in queue.
    /// \param sendNotifyCallback function to call when a message sent to queue.
    /// The Notifycallback is not allowed to call any ThreadX API with a suspension option.
    explicit Queue(const std::string_view name, Pool &pool, const Ulong queueSizeInNumOfMessages, const NotifyCallback &sendNotifyCallback = {})
        requires(std::is_base_of_v<BytePoolBase, Pool>);
    explicit Queue(const std::string_view name, Pool &pool, const NotifyCallback &sendNotifyCallback = {})
        requires(std::is_base_of_v<BlockPoolBase, Pool>);

    ~Queue();

    auto receive();

    // must be used for calls from initialization, timers, and ISRs
    auto tryReceive();

    template <class Clock, typename Duration>
    auto tryReceiveUntil(const std::chrono::time_point<Clock, Duration> &time);

    /// receive a message from queue
    /// \param duration
    /// \return
    template <typename Rep, typename Period>
    auto tryReceiveFor(const std::chrono::duration<Rep, Period> &duration) -> std::expected<Msg, Error>;

    auto send(const Msg &message);

    // must be used for calls from initialization, timers, and ISRs
    auto trySend(const Msg &message);

    template <class Clock, typename Duration>
    auto trySendUntil(const Msg &message, const std::chrono::time_point<Clock, Duration> &time);

    ///
    /// \param duration
    /// \param message
    /// \return
    template <typename Rep, typename Period>
    auto trySendFor(const Msg &message, const std::chrono::duration<Rep, Period> &duration);

    auto sendFront(const Msg &message);

    // must be used for calls from initialization, timers, and ISRs
    auto trySendFront(const Msg &message);

    template <class Clock, typename Duration>
    auto trySendFrontUntil(const Msg &message, const std::chrono::time_point<Clock, Duration> &time);
    ///
    /// \param duration
    /// \param message
    /// \return
    template <typename Rep, typename Period>
    auto trySendFrontFor(const Msg &message, const std::chrono::duration<Rep, Period> &duration);

    /// This service places the highest priority thread suspended for a message (or to place a message) on this queue at
    /// the front of the suspension list. All other threads remain in the same FIFO order they were suspended in.
    auto prioritise();
    /// delete all messages
    auto flush();

    auto name() const;

  private:
    static auto sendNotifyCallback(auto queuePtr);
    auto init(const std::string_view name, const Ulong queueSizeInBytes);

    Allocation<Pool> m_queueAlloc;
    const NotifyCallback m_sendNotifyCallback;
};

template <typename Msg, class Pool>
Queue<Msg, Pool>::Queue(const std::string_view name, Pool &pool, const Ulong queueSizeInNumOfMessages, const NotifyCallback &sendNotifyCallback)
    requires(std::is_base_of_v<BytePoolBase, Pool>)
    : Native::TX_QUEUE{}, m_queueAlloc{pool, queueSizeInNumOfMessages * sizeof(Msg)}, m_sendNotifyCallback{sendNotifyCallback}
{
    init(name, queueSizeInNumOfMessages * sizeof(Msg));
}

template <typename Msg, class Pool>
Queue<Msg, Pool>::Queue(const std::string_view name, Pool &pool, const NotifyCallback &sendNotifyCallback)
    requires(std::is_base_of_v<BlockPoolBase, Pool>)
    : Native::TX_QUEUE{}, m_queueAlloc{pool}, m_sendNotifyCallback{sendNotifyCallback}
{
    assert(pool.blockSize() % sizeof(Msg) == 0);

    init(name, pool.blockSize());
}

template <typename Msg, class Pool>
auto Queue<Msg, Pool>::init(const std::string_view name, const Ulong queueSizeInBytes)
{
    static_assert(sizeof(Msg) % sizeof(wordSize) == 0, "Queue message size must be a multiple of word size.");

    using namespace Native;
    [[maybe_unused]] Error error{tx_queue_create(this, const_cast<char *>(name.data()), sizeof(Msg) / sizeof(wordSize), m_queueAlloc.get(), queueSizeInBytes)};
    assert(error == Error::success);

    if (m_sendNotifyCallback)
    {
        error = Error{tx_queue_send_notify(this, Queue::sendNotifyCallback)};
        assert(error == Error::success);
    }
}

template <typename Msg, class Pool>
Queue<Msg, Pool>::~Queue()
{
    [[maybe_unused]] Error error{tx_queue_delete(this)};
    assert(error == Error::success);
}

template <typename Msg, class Pool>
constexpr size_t Queue<Msg, Pool>::messageSize()
{
    return sizeof(Msg);
}

template <typename Msg, class Pool>
auto Queue<Msg, Pool>::receive()
{
    return tryReceiveFor(TickTimer::waitForever);
}

// must be used for calls from initialization, timers, and ISRs
template <typename Msg, class Pool>
auto Queue<Msg, Pool>::tryReceive()
{
    return tryReceiveFor(TickTimer::noWait);
}

template <typename Msg, class Pool>
template <class Clock, typename Duration>
auto Queue<Msg, Pool>::tryReceiveUntil(const std::chrono::time_point<Clock, Duration> &time)
{
    return tryReceiveFor(time - Clock::now());
}

template <typename Msg, class Pool>
template <typename Rep, typename Period>
auto Queue<Msg, Pool>::tryReceiveFor(const std::chrono::duration<Rep, Period> &duration) -> std::expected<Msg, Error>
{
    Msg message;
    if (Error error{tx_queue_receive(this, std::addressof(message), TickTimer::ticks(duration))}; error != Error::success)
    {
        return std::unexpected(error);
    }

    return message;
}

template <typename Msg, class Pool>
auto Queue<Msg, Pool>::send(const Msg &message)
{
    return trySendFor(message, TickTimer::waitForever);
}

// must be used for calls from initialization, timers, and ISRs
template <typename Msg, class Pool>
auto Queue<Msg, Pool>::trySend(const Msg &message)
{
    return trySendFor(message, TickTimer::noWait);
}

template <typename Msg, class Pool>
template <class Clock, typename Duration>
auto Queue<Msg, Pool>::trySendUntil(const Msg &message, const std::chrono::time_point<Clock, Duration> &time)
{
    return trySendFor(message, time - Clock::now());
}

///
/// \param duration
/// \param message
/// \return
template <typename Msg, class Pool>
template <typename Rep, typename Period>
auto Queue<Msg, Pool>::trySendFor(const Msg &message, const std::chrono::duration<Rep, Period> &duration)
{
    return Error{tx_queue_send(this, std::addressof(const_cast<Msg &>(message)), TickTimer::ticks(duration))};
}

template <typename Msg, class Pool>
auto Queue<Msg, Pool>::sendFront(const Msg &message)
{
    return trySendFrontFor(message, TickTimer::waitForever);
}

// must be used for calls from initialization, timers, and ISRs
template <typename Msg, class Pool>
auto Queue<Msg, Pool>::trySendFront(const Msg &message)
{
    return trySendFrontFor(message, TickTimer::noWait);
}

template <typename Msg, class Pool>
template <class Clock, typename Duration>
auto Queue<Msg, Pool>::trySendFrontUntil(const Msg &message, const std::chrono::time_point<Clock, Duration> &time)
{
    return trySendFrontFor(message, time - Clock::now());
}

///
/// \param duration
/// \param message
/// \return
template <typename Msg, class Pool>
template <typename Rep, typename Period>
auto Queue<Msg, Pool>::trySendFrontFor(const Msg &message, const std::chrono::duration<Rep, Period> &duration)
{
    return Error{tx_queue_front_send(this, std::addressof(const_cast<Msg &>(message)), TickTimer::ticks(duration))};
}

template <typename Msg, class Pool>
auto Queue<Msg, Pool>::prioritise()
{
    return Error{tx_queue_prioritize(this)};
}

template <typename Msg, class Pool>
auto Queue<Msg, Pool>::flush()
{
    return Error{tx_queue_flush(this)};
}

template <typename Msg, class Pool>
auto Queue<Msg, Pool>::name() const
{
    return std::string_view{tx_queue_name};
}

template <typename Msg, class Pool>
auto Queue<Msg, Pool>::sendNotifyCallback(auto queuePtr)
{
    auto &queue{static_cast<Queue &>(*queuePtr)};
    queue.m_sendNotifyCallback(queue);
}
} // namespace ThreadX
