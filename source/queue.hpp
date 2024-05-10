#pragma once

#include "memoryPool.hpp"
#include "tickTimer.hpp"
#include "txCommon.hpp"
#include <functional>

namespace ThreadX
{
class QueueBase : protected Native::TX_QUEUE
{
  public:
    QueueBase(const QueueBase &) = delete;
    QueueBase &operator=(const QueueBase &) = delete;
    /// This service places the highest priority thread suspended for a message (or to place a message) on this queue at
    /// the front of the suspension list. All other threads remain in the same FIFO order they were suspended in.
    Error prioritise();
    /// delete all messages
    Error flush();

    std::string_view name() const;

  protected:
    QueueBase(MemoryPoolBase &pool);
    ~QueueBase();

  private:
    MemoryPoolBase &m_pool;
};

///
/// \tparam Msg message structure type
template <typename Msg> class Queue : public QueueBase
{
  public:
    /// external Notifycallback type
    using NotifyCallback = std::function<void(Queue &)>;
    using MsgPair = std::pair<Error, Msg>;

    static constexpr size_t messageSize();

    ///
    /// \param pool byte pool to allocate queue in.
    /// \param queueSizeInNumOfMessages max num of messages in queue.
    /// \param sendNotifyCallback function to call when a message sent to queue.
    /// The Notifycallback is not allowed to call any ThreadX API with a suspension option.
    Queue(const std::string_view name, BytePoolBase &pool, const Ulong queueSizeInNumOfMessages,
          const NotifyCallback &sendNotifyCallback = {});
    Queue(const std::string_view name, BlockPoolBase &pool, const NotifyCallback sendNotifyCallback = {});

    auto receive();

    // must be used for calls from initialization, timers, and ISRs
    auto tryReceive();

    template <class Clock, typename Duration>
    auto tryReceiveUntil(const std::chrono::time_point<Clock, Duration> &time);

    /// receive a message from queue
    /// \param duration
    /// \return
    template <typename Rep, typename Period> MsgPair tryReceiveFor(const std::chrono::duration<Rep, Period> &duration);

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

  private:
    auto create(const std::string_view name, const Ulong queueSizeInBytes, void *const queueStartPtr);
    static auto sendNotifyCallback(auto queuePtr);

    const NotifyCallback m_sendNotifyCallback;
};

template <typename Msg> constexpr size_t Queue<Msg>::messageSize()
{
    return sizeof(Msg);
}

template <typename Msg>
Queue<Msg>::Queue(const std::string_view name, BytePoolBase &pool, const Ulong queueSizeInNumOfMessages,
                  const NotifyCallback &sendNotifyCallback)
    : QueueBase{pool}, m_sendNotifyCallback{sendNotifyCallback}
{
    Ulong queueSizeInBytes{queueSizeInNumOfMessages * sizeof(Msg)};

    auto [error, queueStartPtr] = pool.allocate(queueSizeInBytes);
    assert(error == Error::success);

    error = create(name, queueSizeInBytes, queueStartPtr);
    assert(error == Error::success);
}

template <typename Msg>
Queue<Msg>::Queue(const std::string_view name, BlockPoolBase &pool, const NotifyCallback sendNotifyCallback)
    : QueueBase{pool}, m_sendNotifyCallback{sendNotifyCallback}
{
    auto [error, queueStartPtr] = pool.allocate();
    assert(error == Error::success);

    error = create(name, pool.blockSize(), queueStartPtr);
    assert(error == Error::success);
}

template <typename Msg>
auto Queue<Msg>::create(const std::string_view name, const Ulong queueSizeInBytes, void *const queueStartPtr)
{
    static_assert(sizeof(Msg) % sizeof(sizeOfUlong) == 0, "Queue message size must be a multiple of word (32-bit).");

    using namespace Native;
    Error error{tx_queue_create(
        this, const_cast<char *>(name.data()), sizeof(Msg) / sizeof(sizeOfUlong), queueStartPtr, queueSizeInBytes)};
    assert(error == Error::success);

    if (m_sendNotifyCallback)
    {
        error = Error{tx_queue_send_notify(this, Queue::sendNotifyCallback)};
        assert(error == Error::success);
    }

    return error;
}

template <typename Msg> auto Queue<Msg>::receive()
{
    return tryReceiveFor(TickTimer::waitForever);
}

// must be used for calls from initialization, timers, and ISRs
template <typename Msg> auto Queue<Msg>::tryReceive()
{
    return tryReceiveFor(TickTimer::noWait);
}

template <typename Msg>
template <class Clock, typename Duration>
auto Queue<Msg>::tryReceiveUntil(const std::chrono::time_point<Clock, Duration> &time)
{
    return tryReceiveFor(time - Clock::now());
}

template <typename Msg>
template <typename Rep, typename Period>
Queue<Msg>::MsgPair Queue<Msg>::tryReceiveFor(const std::chrono::duration<Rep, Period> &duration)
{
    Msg message;
    Error error{tx_queue_receive(
        this, std::addressof(message), TickTimer::ticks(std::chrono::duration_cast<TickTimer::Duration>(duration)))};
    return {error, message};
}

template <typename Msg> auto Queue<Msg>::send(const Msg &message)
{
    return trySendFor(message, TickTimer::waitForever);
}

// must be used for calls from initialization, timers, and ISRs
template <typename Msg> auto Queue<Msg>::trySend(const Msg &message)
{
    return trySendFor(message, TickTimer::noWait);
}

template <typename Msg>
template <class Clock, typename Duration>
auto Queue<Msg>::trySendUntil(const Msg &message, const std::chrono::time_point<Clock, Duration> &time)
{
    return trySendFor(message, time - Clock::now());
}

///
/// \param duration
/// \param message
/// \return
template <typename Msg>
template <typename Rep, typename Period>
auto Queue<Msg>::trySendFor(const Msg &message, const std::chrono::duration<Rep, Period> &duration)
{
    return Error{tx_queue_send(this, std::addressof(const_cast<Msg &>(message)),
                               TickTimer::ticks(std::chrono::duration_cast<TickTimer::Duration>(duration)))};
}

template <typename Msg> auto Queue<Msg>::sendFront(const Msg &message)
{
    return trySendFrontFor(message, TickTimer::waitForever);
}

// must be used for calls from initialization, timers, and ISRs
template <typename Msg> auto Queue<Msg>::trySendFront(const Msg &message)
{
    return trySendFrontFor(message, TickTimer::noWait);
}

template <typename Msg>
template <class Clock, typename Duration>
auto Queue<Msg>::trySendFrontUntil(const Msg &message, const std::chrono::time_point<Clock, Duration> &time)
{
    return trySendFrontFor(message, time - Clock::now());
}

///
/// \param duration
/// \param message
/// \return
template <typename Msg>
template <typename Rep, typename Period>
auto Queue<Msg>::trySendFrontFor(const Msg &message, const std::chrono::duration<Rep, Period> &duration)
{
    return Error{tx_queue_front_send(this, std::addressof(const_cast<Msg &>(message)),
                                     TickTimer::ticks(std::chrono::duration_cast<TickTimer::Duration>(duration)))};
}

template <typename Msg> auto Queue<Msg>::sendNotifyCallback(auto queuePtr)
{
    auto &queue{static_cast<Queue &>(*queuePtr)};
    queue.m_sendNotifyCallback(queue);
}
} // namespace ThreadX
