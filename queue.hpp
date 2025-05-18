#pragma once

#include "allocator.hpp"
#include "tickTimer.hpp"
#include "txCommon.hpp"
#include <cassert>
#include <chrono>
#include <expected>
#include <functional>
#include <string_view>

namespace ThreadX
{
template <typename Message, SimpleAllocator Allocator>
class Queue final : Native::TX_QUEUE
{
    static_assert(sizeof(Message) % sizeof(wordSize) == 0, "Queue message size must be a multiple of word size.");

  public:
    /// external Notifycallback type
    using NotifyCallback = std::function<void(Queue &)>;
    using ExpectedMessage = std::expected<Message, Error>;

    Queue(const Queue &) = delete;
    Queue &operator=(const Queue &) = delete;

    [[nodiscard]] static consteval auto messageSize() -> size_t;

    /// Constructor
    /// \param name name of queue
    /// \param allocator allocator to use for queue memory
    /// \param size max num of messages in queue.
    /// \param sendNotifyCallback function to call when a message sent to queue.
    /// The Notifycallback is not allowed to call any ThreadX API with a suspension option.
    explicit Queue(const std::string_view name, Allocator &allocator, const Ulong size, const NotifyCallback &sendNotifyCallback = {})
        requires(sizeof(typename Allocator::value_type) == sizeof(std::byte));

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

    [[nodiscard]] auto name() const -> std::string_view;

  private:
    static auto sendNotifyCallback(auto queuePtr) -> void;

    Allocator &m_allocator;
    const NotifyCallback m_sendNotifyCallback;
};

template <typename Message, SimpleAllocator Allocator>
Queue<Message, Allocator>::Queue(const std::string_view name, Allocator &allocator, const Ulong size, const NotifyCallback &sendNotifyCallback)
    requires(sizeof(typename Allocator::value_type) == sizeof(std::byte))
    : Native::TX_QUEUE{}, m_allocator{allocator}, m_sendNotifyCallback{sendNotifyCallback}
{
    using namespace Native;
    [[maybe_unused]] Error error{tx_queue_create(this, const_cast<char *>(name.data()), sizeof(Message) / sizeof(wordSize),
                                                 m_allocator.allocate(size * sizeof(Message)), size * sizeof(Message))};
    assert(error == Error::success);

    if (m_sendNotifyCallback)
    {
        error = Error{tx_queue_send_notify(this, Queue::sendNotifyCallback)};
        assert(error == Error::success);
    }
}

template <typename Message, SimpleAllocator Allocator>
Queue<Message, Allocator>::~Queue()
{
    [[maybe_unused]] Error error{tx_queue_delete(this)};
    assert(error == Error::success);

    m_allocator.deallocate(reinterpret_cast<Allocator::value_type *>(tx_queue_start), tx_queue_capacity * sizeof(Message));
}

template <typename Message, SimpleAllocator Allocator>
consteval auto Queue<Message, Allocator>::messageSize() -> size_t
{
    return sizeof(Message);
}

template <typename Message, SimpleAllocator Allocator>
auto Queue<Message, Allocator>::receive() -> ExpectedMessage
{
    return tryReceiveFor(TickTimer::waitForever);
}

// must be used for calls from initialization, timers, and ISRs
template <typename Message, SimpleAllocator Allocator>
auto Queue<Message, Allocator>::tryReceive() -> ExpectedMessage
{
    return tryReceiveFor(TickTimer::noWait);
}

template <typename Message, SimpleAllocator Allocator>
template <class Clock, typename Duration>
auto Queue<Message, Allocator>::tryReceiveUntil(const std::chrono::time_point<Clock, Duration> &time) -> ExpectedMessage
{
    return tryReceiveFor(time - Clock::now());
}

template <typename Message, SimpleAllocator Allocator>
template <typename Rep, typename Period>
auto Queue<Message, Allocator>::tryReceiveFor(const std::chrono::duration<Rep, Period> &duration) -> ExpectedMessage
{
    Message message;
    if (Error error{tx_queue_receive(this, std::addressof(message), TickTimer::ticks(duration))}; error != Error::success)
    {
        return std::unexpected(error);
    }

    return message;
}

template <typename Message, SimpleAllocator Allocator>
auto Queue<Message, Allocator>::send(const Message &message) -> Error
{
    return trySendFor(message, TickTimer::waitForever);
}

// must be used for calls from initialization, timers, and ISRs
template <typename Message, SimpleAllocator Allocator>
auto Queue<Message, Allocator>::trySend(const Message &message) -> Error
{
    return trySendFor(message, TickTimer::noWait);
}

template <typename Message, SimpleAllocator Allocator>
template <class Clock, typename Duration>
auto Queue<Message, Allocator>::trySendUntil(const Message &message, const std::chrono::time_point<Clock, Duration> &time) -> Error
{
    return trySendFor(message, time - Clock::now());
}

///
/// \param duration
/// \param message
/// \return
template <typename Message, SimpleAllocator Allocator>
template <typename Rep, typename Period>
auto Queue<Message, Allocator>::trySendFor(const Message &message, const std::chrono::duration<Rep, Period> &duration) -> Error
{
    return Error{tx_queue_send(this, std::addressof(const_cast<Message &>(message)), TickTimer::ticks(duration))};
}

template <typename Message, SimpleAllocator Allocator>
auto Queue<Message, Allocator>::sendFront(const Message &message) -> Error
{
    return trySendFrontFor(message, TickTimer::waitForever);
}

// must be used for calls from initialization, timers, and ISRs
template <typename Message, SimpleAllocator Allocator>
auto Queue<Message, Allocator>::trySendFront(const Message &message) -> Error
{
    return trySendFrontFor(message, TickTimer::noWait);
}

template <typename Message, SimpleAllocator Allocator>
template <class Clock, typename Duration>
auto Queue<Message, Allocator>::trySendFrontUntil(const Message &message, const std::chrono::time_point<Clock, Duration> &time) -> Error
{
    return trySendFrontFor(message, time - Clock::now());
}

///
/// \param duration
/// \param message
/// \return
template <typename Message, SimpleAllocator Allocator>
template <typename Rep, typename Period>
auto Queue<Message, Allocator>::trySendFrontFor(const Message &message, const std::chrono::duration<Rep, Period> &duration) -> Error
{
    return Error{tx_queue_front_send(this, std::addressof(const_cast<Message &>(message)), TickTimer::ticks(duration))};
}

template <typename Message, SimpleAllocator Allocator>
auto Queue<Message, Allocator>::prioritise() -> Error
{
    return Error{tx_queue_prioritize(this)};
}

template <typename Message, SimpleAllocator Allocator>
auto Queue<Message, Allocator>::flush() -> Error
{
    return Error{tx_queue_flush(this)};
}

template <typename Message, SimpleAllocator Allocator>
auto Queue<Message, Allocator>::name() const -> std::string_view
{
    return std::string_view{tx_queue_name};
}

template <typename Message, SimpleAllocator Allocator>
auto Queue<Message, Allocator>::sendNotifyCallback(auto queuePtr) -> void
{
    auto &queue{static_cast<Queue &>(*queuePtr)};
    queue.m_sendNotifyCallback(queue);
}
} // namespace ThreadX
