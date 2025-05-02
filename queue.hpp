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
template <typename Message, class Pool>
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
    auto tryReceiveFor(const std::chrono::duration<Rep, Period> &duration) -> std::expected<Message, Error>;

    auto send(const Message &message);

    // must be used for calls from initialization, timers, and ISRs
    auto trySend(const Message &message);

    template <class Clock, typename Duration>
    auto trySendUntil(const Message &message, const std::chrono::time_point<Clock, Duration> &time);

    ///
    /// \param duration
    /// \param message
    /// \return
    template <typename Rep, typename Period>
    auto trySendFor(const Message &message, const std::chrono::duration<Rep, Period> &duration);

    auto sendFront(const Message &message);

    // must be used for calls from initialization, timers, and ISRs
    auto trySendFront(const Message &message);

    template <class Clock, typename Duration>
    auto trySendFrontUntil(const Message &message, const std::chrono::time_point<Clock, Duration> &time);
    ///
    /// \param duration
    /// \param message
    /// \return
    template <typename Rep, typename Period>
    auto trySendFrontFor(const Message &message, const std::chrono::duration<Rep, Period> &duration);

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

template <typename Message, class Pool>
Queue<Message, Pool>::Queue(const std::string_view name, Pool &pool, const Ulong queueSizeInNumOfMessages, const NotifyCallback &sendNotifyCallback)
    requires(std::is_base_of_v<BytePoolBase, Pool>)
    : Native::TX_QUEUE{}, m_queueAlloc{pool, queueSizeInNumOfMessages * sizeof(Message)}, m_sendNotifyCallback{sendNotifyCallback}
{
    init(name, queueSizeInNumOfMessages * sizeof(Message));
}

template <typename Message, class Pool>
Queue<Message, Pool>::Queue(const std::string_view name, Pool &pool, const NotifyCallback &sendNotifyCallback)
    requires(std::is_base_of_v<BlockPoolBase, Pool>)
    : Native::TX_QUEUE{}, m_queueAlloc{pool}, m_sendNotifyCallback{sendNotifyCallback}
{
    assert(pool.blockSize() % sizeof(Message) == 0);

    init(name, pool.blockSize());
}

template <typename Message, class Pool>
auto Queue<Message, Pool>::init(const std::string_view name, const Ulong queueSizeInBytes)
{
    static_assert(sizeof(Message) % sizeof(wordSize) == 0, "Queue message size must be a multiple of word size.");

    using namespace Native;
    [[maybe_unused]] Error error{tx_queue_create(this, const_cast<char *>(name.data()), sizeof(Message) / sizeof(wordSize), m_queueAlloc.get(), queueSizeInBytes)};
    assert(error == Error::success);

    if (m_sendNotifyCallback)
    {
        error = Error{tx_queue_send_notify(this, Queue::sendNotifyCallback)};
        assert(error == Error::success);
    }
}

template <typename Message, class Pool>
Queue<Message, Pool>::~Queue()
{
    [[maybe_unused]] Error error{tx_queue_delete(this)};
    assert(error == Error::success);
}

template <typename Message, class Pool>
constexpr size_t Queue<Message, Pool>::messageSize()
{
    return sizeof(Message);
}

template <typename Message, class Pool>
auto Queue<Message, Pool>::receive()
{
    return tryReceiveFor(TickTimer::waitForever);
}

// must be used for calls from initialization, timers, and ISRs
template <typename Message, class Pool>
auto Queue<Message, Pool>::tryReceive()
{
    return tryReceiveFor(TickTimer::noWait);
}

template <typename Message, class Pool>
template <class Clock, typename Duration>
auto Queue<Message, Pool>::tryReceiveUntil(const std::chrono::time_point<Clock, Duration> &time)
{
    return tryReceiveFor(time - Clock::now());
}

template <typename Message, class Pool>
template <typename Rep, typename Period>
auto Queue<Message, Pool>::tryReceiveFor(const std::chrono::duration<Rep, Period> &duration) -> std::expected<Message, Error>
{
    Message message;
    if (Error error{tx_queue_receive(this, std::addressof(message), TickTimer::ticks(duration))}; error != Error::success)
    {
        return std::unexpected(error);
    }

    return message;
}

template <typename Message, class Pool>
auto Queue<Message, Pool>::send(const Message &message)
{
    return trySendFor(message, TickTimer::waitForever);
}

// must be used for calls from initialization, timers, and ISRs
template <typename Message, class Pool>
auto Queue<Message, Pool>::trySend(const Message &message)
{
    return trySendFor(message, TickTimer::noWait);
}

template <typename Message, class Pool>
template <class Clock, typename Duration>
auto Queue<Message, Pool>::trySendUntil(const Message &message, const std::chrono::time_point<Clock, Duration> &time)
{
    return trySendFor(message, time - Clock::now());
}

///
/// \param duration
/// \param message
/// \return
template <typename Message, class Pool>
template <typename Rep, typename Period>
auto Queue<Message, Pool>::trySendFor(const Message &message, const std::chrono::duration<Rep, Period> &duration)
{
    return Error{tx_queue_send(this, std::addressof(const_cast<Message &>(message)), TickTimer::ticks(duration))};
}

template <typename Message, class Pool>
auto Queue<Message, Pool>::sendFront(const Message &message)
{
    return trySendFrontFor(message, TickTimer::waitForever);
}

// must be used for calls from initialization, timers, and ISRs
template <typename Message, class Pool>
auto Queue<Message, Pool>::trySendFront(const Message &message)
{
    return trySendFrontFor(message, TickTimer::noWait);
}

template <typename Message, class Pool>
template <class Clock, typename Duration>
auto Queue<Message, Pool>::trySendFrontUntil(const Message &message, const std::chrono::time_point<Clock, Duration> &time)
{
    return trySendFrontFor(message, time - Clock::now());
}

///
/// \param duration
/// \param message
/// \return
template <typename Message, class Pool>
template <typename Rep, typename Period>
auto Queue<Message, Pool>::trySendFrontFor(const Message &message, const std::chrono::duration<Rep, Period> &duration)
{
    return Error{tx_queue_front_send(this, std::addressof(const_cast<Message &>(message)), TickTimer::ticks(duration))};
}

template <typename Message, class Pool>
auto Queue<Message, Pool>::prioritise()
{
    return Error{tx_queue_prioritize(this)};
}

template <typename Message, class Pool>
auto Queue<Message, Pool>::flush()
{
    return Error{tx_queue_flush(this)};
}

template <typename Message, class Pool>
auto Queue<Message, Pool>::name() const
{
    return std::string_view{tx_queue_name};
}

template <typename Message, class Pool>
auto Queue<Message, Pool>::sendNotifyCallback(auto queuePtr)
{
    auto &queue{static_cast<Queue &>(*queuePtr)};
    queue.m_sendNotifyCallback(queue);
}
} // namespace ThreadX
