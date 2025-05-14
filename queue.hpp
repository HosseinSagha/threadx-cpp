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
class Queue final : Native::TX_QUEUE, Allocation<Pool>
{
  public:
    /// external Notifycallback type
    using NotifyCallback = std::function<void(Queue &)>;
    using ExpectedMessage = std::expected<Message, Error>;

    Queue(const Queue &) = delete;
    Queue &operator=(const Queue &) = delete;

    static consteval auto messageSize() -> size_t;

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

    auto receive() -> ExpectedMessage;

    // must be used for calls from initialization, timers, and ISRs
    auto tryReceive() -> ExpectedMessage;

    template <class Clock, typename Duration>
    auto tryReceiveUntil(const std::chrono::time_point<Clock, Duration> &time) -> ExpectedMessage;
    /// receive a message from queue
    /// \param duration
    /// \return
    template <typename Rep, typename Period>
    auto tryReceiveFor(const std::chrono::duration<Rep, Period> &duration) -> ExpectedMessage;

    auto send(const Message &message) -> Error;

    // must be used for calls from initialization, timers, and ISRs
    auto trySend(const Message &message) -> Error;

    template <class Clock, typename Duration>
    auto trySendUntil(const Message &message, const std::chrono::time_point<Clock, Duration> &time) -> Error;

    ///
    /// \param duration
    /// \param message
    /// \return
    template <typename Rep, typename Period>
    auto trySendFor(const Message &message, const std::chrono::duration<Rep, Period> &duration) -> Error;

    auto sendFront(const Message &message) -> Error;

    // must be used for calls from initialization, timers, and ISRs
    auto trySendFront(const Message &message) -> Error;

    template <class Clock, typename Duration>
    auto trySendFrontUntil(const Message &message, const std::chrono::time_point<Clock, Duration> &time) -> Error;
    ///
    /// \param duration
    /// \param message
    /// \return
    template <typename Rep, typename Period>
    auto trySendFrontFor(const Message &message, const std::chrono::duration<Rep, Period> &duration) -> Error;

    /// This service places the highest priority thread suspended for a message (or to place a message) on this queue at
    /// the front of the suspension list. All other threads remain in the same FIFO order they were suspended in.
    auto prioritise() -> Error;
    /// delete all messages
    auto flush() -> Error;

    auto name() const -> std::string_view;

  private:
    static auto sendNotifyCallback(auto queuePtr) -> void;
    auto init(const std::string_view name, const Ulong queueSizeInBytes) -> void;

    const NotifyCallback m_sendNotifyCallback;
};

template <typename Message, class Pool>
Queue<Message, Pool>::Queue(const std::string_view name, Pool &pool, const Ulong queueSizeInNumOfMessages, const NotifyCallback &sendNotifyCallback)
    requires(std::is_base_of_v<BytePoolBase, Pool>)
    : Native::TX_QUEUE{}, Allocation<Pool>{pool, queueSizeInNumOfMessages * sizeof(Message)}, m_sendNotifyCallback{sendNotifyCallback}
{
    init(name, queueSizeInNumOfMessages * sizeof(Message));
}

template <typename Message, class Pool>
Queue<Message, Pool>::Queue(const std::string_view name, Pool &pool, const NotifyCallback &sendNotifyCallback)
    requires(std::is_base_of_v<BlockPoolBase, Pool>)
    : Native::TX_QUEUE{}, Allocation<Pool>{pool}, m_sendNotifyCallback{sendNotifyCallback}
{
    static_assert(pool.blockSize() % sizeof(Message) == 0);

    init(name, pool.blockSize());
}

template <typename Message, class Pool>
auto Queue<Message, Pool>::init(const std::string_view name, const Ulong queueSizeInBytes) -> void
{
    static_assert(sizeof(Message) % sizeof(wordSize) == 0, "Queue message size must be a multiple of word size.");

    using namespace Native;
    [[maybe_unused]] Error error{tx_queue_create(this, const_cast<char *>(name.data()), sizeof(Message) / sizeof(wordSize), Allocation<Pool>::allocationPtr(), queueSizeInBytes)};
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
consteval auto Queue<Message, Pool>::messageSize() -> size_t
{
    return sizeof(Message);
}

template <typename Message, class Pool>
auto Queue<Message, Pool>::receive() -> ExpectedMessage
{
    return tryReceiveFor(TickTimer::waitForever);
}

// must be used for calls from initialization, timers, and ISRs
template <typename Message, class Pool>
auto Queue<Message, Pool>::tryReceive() -> ExpectedMessage
{
    return tryReceiveFor(TickTimer::noWait);
}

template <typename Message, class Pool>
template <class Clock, typename Duration>
auto Queue<Message, Pool>::tryReceiveUntil(const std::chrono::time_point<Clock, Duration> &time) -> ExpectedMessage
{
    return tryReceiveFor(time - Clock::now());
}

template <typename Message, class Pool>
template <typename Rep, typename Period>
auto Queue<Message, Pool>::tryReceiveFor(const std::chrono::duration<Rep, Period> &duration) -> ExpectedMessage
{
    Message message;
    if (Error error{tx_queue_receive(this, std::addressof(message), TickTimer::ticks(duration))}; error != Error::success)
    {
        return std::unexpected(error);
    }

    return message;
}

template <typename Message, class Pool>
auto Queue<Message, Pool>::send(const Message &message) -> Error
{
    return trySendFor(message, TickTimer::waitForever);
}

// must be used for calls from initialization, timers, and ISRs
template <typename Message, class Pool>
auto Queue<Message, Pool>::trySend(const Message &message) -> Error
{
    return trySendFor(message, TickTimer::noWait);
}

template <typename Message, class Pool>
template <class Clock, typename Duration>
auto Queue<Message, Pool>::trySendUntil(const Message &message, const std::chrono::time_point<Clock, Duration> &time) -> Error
{
    return trySendFor(message, time - Clock::now());
}

///
/// \param duration
/// \param message
/// \return
template <typename Message, class Pool>
template <typename Rep, typename Period>
auto Queue<Message, Pool>::trySendFor(const Message &message, const std::chrono::duration<Rep, Period> &duration) -> Error
{
    return Error{tx_queue_send(this, std::addressof(const_cast<Message &>(message)), TickTimer::ticks(duration))};
}

template <typename Message, class Pool>
auto Queue<Message, Pool>::sendFront(const Message &message) -> Error
{
    return trySendFrontFor(message, TickTimer::waitForever);
}

// must be used for calls from initialization, timers, and ISRs
template <typename Message, class Pool>
auto Queue<Message, Pool>::trySendFront(const Message &message) -> Error
{
    return trySendFrontFor(message, TickTimer::noWait);
}

template <typename Message, class Pool>
template <class Clock, typename Duration>
auto Queue<Message, Pool>::trySendFrontUntil(const Message &message, const std::chrono::time_point<Clock, Duration> &time) -> Error
{
    return trySendFrontFor(message, time - Clock::now());
}

///
/// \param duration
/// \param message
/// \return
template <typename Message, class Pool>
template <typename Rep, typename Period>
auto Queue<Message, Pool>::trySendFrontFor(const Message &message, const std::chrono::duration<Rep, Period> &duration) -> Error
{
    return Error{tx_queue_front_send(this, std::addressof(const_cast<Message &>(message)), TickTimer::ticks(duration))};
}

template <typename Message, class Pool>
auto Queue<Message, Pool>::prioritise() -> Error
{
    return Error{tx_queue_prioritize(this)};
}

template <typename Message, class Pool>
auto Queue<Message, Pool>::flush() -> Error
{
    return Error{tx_queue_flush(this)};
}

template <typename Message, class Pool>
auto Queue<Message, Pool>::name() const -> std::string_view
{
    return std::string_view{tx_queue_name};
}

template <typename Message, class Pool>
auto Queue<Message, Pool>::sendNotifyCallback(auto queuePtr) -> void
{
    auto &queue{static_cast<Queue &>(*queuePtr)};
    queue.m_sendNotifyCallback(queue);
}
} // namespace ThreadX
