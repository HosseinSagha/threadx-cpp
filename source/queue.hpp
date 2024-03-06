#pragma once

#include "memoryPool.hpp"
#include "timer.hpp"
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

  protected:
    QueueBase(MemoryPoolBase &pool);
    ~QueueBase();

    void *m_queueStartPtr{};

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
    using ReturnTuple = std::tuple<Error, Msg>;

    static constexpr Ulong messageSize();

    ///
    /// \param pool byte pool to allocate queue in.
    /// \param queueSizeInNumOfMessages max num of messages in queue.
    /// \param sendNotifyCallback function to call when a message sent to queue.
    /// The Notifycallback is not allowed to call any ThreadX API with a suspension option.
    Queue(BytePoolBase &pool, const Ulong queueSizeInNumOfMessages, const NotifyCallback &sendNotifyCallback = {});
    Queue(BlockPoolBase &pool, const NotifyCallback sendNotifyCallback = {});

    auto receive();

    // must be used for calls from initialization, timers, and ISRs
    auto tryReceive();

    auto tryReceiveUntil(const TickTimer::TimePoint &timePoint);

    /// receive a message from queue
    /// \param waitDuration
    /// \return
    ReturnTuple tryReceiveFor(const TickTimer::Duration &waitDuration);

    auto send(const Msg &message);

    // must be used for calls from initialization, timers, and ISRs
    auto trySend(const Msg &message);

    auto trySendUntil(const TickTimer::TimePoint &timePoint, const Msg &message);
    ///
    /// \param waitDuration
    /// \param message
    /// \return
    auto trySendFor(const TickTimer::Duration &waitDuration, const Msg &message);

    auto frontSend(const Msg &message);

    // must be used for calls from initialization, timers, and ISRs
    auto tryFrontSend(const Msg &message);

    auto tryFrontSendUntil(const TickTimer::TimePoint &timePoint, const Msg &message);
    ///
    /// \param waitDuration
    /// \param message
    /// \return
    auto tryFrontSendFor(const TickTimer::Duration &waitDuration, const Msg &message);

  private:
    using QueueBase::m_queueStartPtr;

    auto create(const Ulong queueSizeInBytes);
    static auto sendNotifyCallback(auto queuePtr);

    const NotifyCallback m_sendNotifyCallback;
};

template <typename Msg> constexpr Ulong Queue<Msg>::messageSize()
{
    return sizeof(Msg);
}

template <typename Msg>
Queue<Msg>::Queue(BytePoolBase &pool, const Ulong queueSizeInNumOfMessages, const NotifyCallback &sendNotifyCallback)
    : QueueBase{pool}, m_sendNotifyCallback{sendNotifyCallback}
{
    Ulong queueSizeInBytes{queueSizeInNumOfMessages * sizeof(Msg)};

    auto [error, queueStartPtr] = pool.allocate(queueSizeInBytes);
    assert(error == Error::success);

    m_queueStartPtr = queueStartPtr;
    error = create(queueSizeInBytes);
    assert(error == Error::success);
}

template <typename Msg>
Queue<Msg>::Queue(BlockPoolBase &pool, const NotifyCallback sendNotifyCallback)
    : QueueBase{pool}, m_sendNotifyCallback{sendNotifyCallback}
{
    auto [error, queueStartPtr] = pool.allocate();
    assert(error == Error::success);

    m_queueStartPtr = queueStartPtr;
    error = create(pool.blockSize());
    assert(error == Error::success);
}

template <typename Msg> auto Queue<Msg>::create(const Ulong queueSizeInBytes)
{
    static_assert(sizeof(Msg) % sizeof(sizeOfUlong) == 0, "Queue message size is not a multiple of word (32-bit).");

    using namespace Native;
    Error error{tx_queue_create(
        this, const_cast<char *>("queue"), sizeof(Msg) / sizeof(sizeOfUlong), m_queueStartPtr, queueSizeInBytes)};
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

template <typename Msg> auto Queue<Msg>::tryReceiveUntil(const TickTimer::TimePoint &timePoint)
{
    return tryReceiveFor(timePoint - TickTimer::now());
}

template <typename Msg> Queue<Msg>::ReturnTuple Queue<Msg>::tryReceiveFor(const TickTimer::Duration &waitDuration)
{
    Msg message;
    Error error{tx_queue_receive(this, std::addressof(message), TickTimer::ticks(waitDuration))};
    return {error, message};
}

template <typename Msg> auto Queue<Msg>::send(const Msg &message)
{
    return trySendFor(TickTimer::waitForever, message);
}

// must be used for calls from initialization, timers, and ISRs
template <typename Msg> auto Queue<Msg>::trySend(const Msg &message)
{
    return trySendFor(TickTimer::noWait, message);
}

template <typename Msg> auto Queue<Msg>::trySendUntil(const TickTimer::TimePoint &timePoint, const Msg &message)
{
    return trySendFor(timePoint - TickTimer::now(), message);
}
///
/// \param waitDuration
/// \param message
/// \return
template <typename Msg> auto Queue<Msg>::trySendFor(const TickTimer::Duration &waitDuration, const Msg &message)
{
    return Error{tx_queue_send(this, std::addressof(const_cast<Msg &>(message)), TickTimer::ticks(waitDuration))};
}

template <typename Msg> auto Queue<Msg>::frontSend(const Msg &message)
{
    return tryFrontSendFor(TickTimer::waitForever, message);
}

// must be used for calls from initialization, timers, and ISRs
template <typename Msg> auto Queue<Msg>::tryFrontSend(const Msg &message)
{
    return tryFrontSendFor(TickTimer::noWait, message);
}

template <typename Msg> auto Queue<Msg>::tryFrontSendUntil(const TickTimer::TimePoint &timePoint, const Msg &message)
{
    return tryFrontSendFor(timePoint - TickTimer::now(), message);
}
///
/// \param waitDuration
/// \param message
/// \return
template <typename Msg> auto Queue<Msg>::tryFrontSendFor(const TickTimer::Duration &waitDuration, const Msg &message)
{
    return Error{tx_queue_front_send(this, std::addressof(const_cast<Msg &>(message)), TickTimer::ticks(waitDuration))};
}

template <typename Msg> auto Queue<Msg>::sendNotifyCallback(auto queuePtr)
{
    auto &queue{static_cast<Queue &>(*queuePtr)};
    queue.m_sendNotifyCallback(queue);
}
} // namespace ThreadX
