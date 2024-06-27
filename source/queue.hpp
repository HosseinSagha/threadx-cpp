#pragma once

#include "memoryPool.hpp"
#include "tickTimer.hpp"
#include "txCommon.hpp"
#include <functional>

namespace ThreadX
{
class QueueBaseBase : protected Native::TX_QUEUE
{
  public:
    QueueBaseBase(const QueueBaseBase &) = delete;
    QueueBaseBase &operator=(const QueueBaseBase &) = delete;
    /// This service places the highest priority thread suspended for a message (or to place a message) on this queue at
    /// the front of the suspension list. All other threads remain in the same FIFO order they were suspended in.
    Error prioritise();
    /// delete all messages
    Error flush();

    std::string_view name() const;

  protected:
    QueueBaseBase();
};

///
/// \tparam Msg message structure type
template <typename Msg> class QueueBase : public QueueBaseBase
{
  public:
    /// external Notifycallback type
    using NotifyCallback = std::function<void(QueueBase &)>;
    using MsgPair = std::pair<Error, Msg>;

    static constexpr size_t messageSize();

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

  protected:
    QueueBase(const auto &sendNotifyCallback);
    auto create(const std::string_view name, const Ulong queueSizeInBytes, void *const queueStartPtr);

  private:
    static auto sendNotifyCallback(auto queuePtr);

    const NotifyCallback m_sendNotifyCallback;
};

template <typename Msg> constexpr size_t QueueBase<Msg>::messageSize()
{
    return sizeof(Msg);
}

template <typename Msg>
QueueBase<Msg>::QueueBase(const auto &sendNotifyCallback) : m_sendNotifyCallback{sendNotifyCallback}
{
}

template <typename Msg>
auto QueueBase<Msg>::create(const std::string_view name, const Ulong queueSizeInBytes, void *const queueStartPtr)
{
    static_assert(sizeof(Msg) % sizeof(wordSize) == 0, "Queue message size must be a multiple of word size.");

    using namespace Native;
    [[maybe_unused]] Error error{tx_queue_create(
        this, const_cast<char *>(name.data()), sizeof(Msg) / sizeof(wordSize), queueStartPtr, queueSizeInBytes)};
    assert(error == Error::success);

    if (m_sendNotifyCallback)
    {
        error = Error{tx_queue_send_notify(this, QueueBase::sendNotifyCallback)};
        assert(error == Error::success);
    }
}

template <typename Msg> auto QueueBase<Msg>::receive()
{
    return tryReceiveFor(TickTimer::waitForever);
}

// must be used for calls from initialization, timers, and ISRs
template <typename Msg> auto QueueBase<Msg>::tryReceive()
{
    return tryReceiveFor(TickTimer::noWait);
}

template <typename Msg>
template <class Clock, typename Duration>
auto QueueBase<Msg>::tryReceiveUntil(const std::chrono::time_point<Clock, Duration> &time)
{
    return tryReceiveFor(time - Clock::now());
}

template <typename Msg>
template <typename Rep, typename Period>
QueueBase<Msg>::MsgPair QueueBase<Msg>::tryReceiveFor(const std::chrono::duration<Rep, Period> &duration)
{
    Msg message;
    Error error{tx_queue_receive(
        this, std::addressof(message), TickTimer::ticks(std::chrono::duration_cast<TickTimer::Duration>(duration)))};
    return {error, message};
}

template <typename Msg> auto QueueBase<Msg>::send(const Msg &message)
{
    return trySendFor(message, TickTimer::waitForever);
}

// must be used for calls from initialization, timers, and ISRs
template <typename Msg> auto QueueBase<Msg>::trySend(const Msg &message)
{
    return trySendFor(message, TickTimer::noWait);
}

template <typename Msg>
template <class Clock, typename Duration>
auto QueueBase<Msg>::trySendUntil(const Msg &message, const std::chrono::time_point<Clock, Duration> &time)
{
    return trySendFor(message, time - Clock::now());
}

///
/// \param duration
/// \param message
/// \return
template <typename Msg>
template <typename Rep, typename Period>
auto QueueBase<Msg>::trySendFor(const Msg &message, const std::chrono::duration<Rep, Period> &duration)
{
    return Error{tx_queue_send(this, std::addressof(const_cast<Msg &>(message)),
                               TickTimer::ticks(std::chrono::duration_cast<TickTimer::Duration>(duration)))};
}

template <typename Msg> auto QueueBase<Msg>::sendFront(const Msg &message)
{
    return trySendFrontFor(message, TickTimer::waitForever);
}

// must be used for calls from initialization, timers, and ISRs
template <typename Msg> auto QueueBase<Msg>::trySendFront(const Msg &message)
{
    return trySendFrontFor(message, TickTimer::noWait);
}

template <typename Msg>
template <class Clock, typename Duration>
auto QueueBase<Msg>::trySendFrontUntil(const Msg &message, const std::chrono::time_point<Clock, Duration> &time)
{
    return trySendFrontFor(message, time - Clock::now());
}

///
/// \param duration
/// \param message
/// \return
template <typename Msg>
template <typename Rep, typename Period>
auto QueueBase<Msg>::trySendFrontFor(const Msg &message, const std::chrono::duration<Rep, Period> &duration)
{
    return Error{tx_queue_front_send(this, std::addressof(const_cast<Msg &>(message)),
                                     TickTimer::ticks(std::chrono::duration_cast<TickTimer::Duration>(duration)))};
}

template <typename Msg> auto QueueBase<Msg>::sendNotifyCallback(auto queuePtr)
{
    auto &queue{static_cast<QueueBase &>(*queuePtr)};
    queue.m_sendNotifyCallback(queue);
}

template <typename Msg, class Pool> class Queue : public QueueBase<Msg>
{
  public:
    ///
    /// \param pool byte pool to allocate queue in.
    /// \param queueSizeInNumOfMessages max num of messages in queue.
    /// \param sendNotifyCallback function to call when a message sent to queue.
    /// The Notifycallback is not allowed to call any ThreadX API with a suspension option.
    Queue(const std::string_view name, Pool &pool, const Ulong queueSizeInNumOfMessages,
          const QueueBase<Msg>::NotifyCallback &sendNotifyCallback = {})
        requires(std::is_base_of_v<BytePoolBase, Pool>);
    Queue(const std::string_view name, Pool &pool, const QueueBase<Msg>::NotifyCallback sendNotifyCallback = {})
        requires(std::is_base_of_v<BlockPoolBase, Pool>);
    ~Queue();

  private:
    using QueueBase<Msg>::create;

    Pool &m_pool;
};

template <typename Msg, class Pool>
Queue<Msg, Pool>::Queue(const std::string_view name, Pool &pool, const Ulong queueSizeInNumOfMessages,
                        const QueueBase<Msg>::NotifyCallback &sendNotifyCallback)
    requires(std::is_base_of_v<BytePoolBase, Pool>)
    : QueueBase<Msg>{sendNotifyCallback}, m_pool{pool}
{
    Ulong queueSizeInBytes{queueSizeInNumOfMessages * sizeof(Msg)};

    auto [error, queueStartPtr] = pool.allocate(queueSizeInBytes);
    assert(error == Error::success);

    QueueBase<Msg>::create(name, queueSizeInBytes, queueStartPtr);
}

template <typename Msg, class Pool>
Queue<Msg, Pool>::Queue(
    const std::string_view name, Pool &pool, const QueueBase<Msg>::NotifyCallback sendNotifyCallback)
    requires(std::is_base_of_v<BlockPoolBase, Pool>)
    : QueueBase<Msg>{sendNotifyCallback}, m_pool{pool}
{
    auto [error, queueStartPtr] = pool.allocate();
    assert(error == Error::success);

    QueueBase<Msg>::create(name, pool.blockSize(), queueStartPtr);
}

template <typename Msg, class Pool> Queue<Msg, Pool>::~Queue()
{
    tx_queue_delete(this);
    m_pool.release(Native::TX_QUEUE::tx_queue_start);
}
} // namespace ThreadX
