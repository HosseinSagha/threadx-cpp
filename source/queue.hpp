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

    std::string_view name();

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
    using ReturnPair = std::pair<Error, Msg>;

    static constexpr size_t messageSize();

    ///
    /// \param pool byte pool to allocate queue in.
    /// \param queueSizeInNumOfMessages max num of messages in queue.
    /// \param sendNotifyCallback function to call when a message sent to queue.
    /// The Notifycallback is not allowed to call any ThreadX API with a suspension option.
    Queue(std::string_view name, BytePoolBase &pool, const Ulong queueSizeInNumOfMessages,
          const NotifyCallback &sendNotifyCallback = {});
    Queue(std::string_view name, BlockPoolBase &pool, const NotifyCallback sendNotifyCallback = {});

    auto receive();

    // must be used for calls from initialization, timers, and ISRs
    auto tryReceive();

    auto tryReceiveUntil(const TickTimer::TimePoint &time);

    /// receive a message from queue
    /// \param waitDuration
    /// \return
    ReturnPair tryReceiveFor(const TickTimer::Duration &waitDuration);

    auto send(const Msg &message);

    // must be used for calls from initialization, timers, and ISRs
    auto trySend(const Msg &message);

    auto trySendUntil(const TickTimer::TimePoint &time, const Msg &message);
    ///
    /// \param waitDuration
    /// \param message
    /// \return
    auto trySendFor(const TickTimer::Duration &waitDuration, const Msg &message);

    auto frontSend(const Msg &message);

    // must be used for calls from initialization, timers, and ISRs
    auto tryFrontSend(const Msg &message);

    auto tryFrontSendUntil(const TickTimer::TimePoint &time, const Msg &message);
    ///
    /// \param waitDuration
    /// \param message
    /// \return
    auto tryFrontSendFor(const TickTimer::Duration &waitDuration, const Msg &message);

  private:
    auto create(std::string_view name, const Ulong queueSizeInBytes, void *const queueStartPtr);
    static auto sendNotifyCallback(auto queuePtr);

    const NotifyCallback m_sendNotifyCallback;
};

template <typename Msg> constexpr size_t Queue<Msg>::messageSize()
{
    return sizeof(Msg);
}

template <typename Msg>
Queue<Msg>::Queue(std::string_view name, BytePoolBase &pool, const Ulong queueSizeInNumOfMessages,
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
Queue<Msg>::Queue(std::string_view name, BlockPoolBase &pool, const NotifyCallback sendNotifyCallback)
    : QueueBase{pool}, m_sendNotifyCallback{sendNotifyCallback}
{
    auto [error, queueStartPtr] = pool.allocate();
    assert(error == Error::success);

    error = create(name, pool.blockSize(), queueStartPtr);
    assert(error == Error::success);
}

template <typename Msg>
auto Queue<Msg>::create(std::string_view name, const Ulong queueSizeInBytes, void *const queueStartPtr)
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

template <typename Msg> auto Queue<Msg>::tryReceiveUntil(const TickTimer::TimePoint &time)
{
    return tryReceiveFor(time - TickTimer::now());
}

template <typename Msg> Queue<Msg>::ReturnPair Queue<Msg>::tryReceiveFor(const TickTimer::Duration &waitDuration)
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

template <typename Msg> auto Queue<Msg>::trySendUntil(const TickTimer::TimePoint &time, const Msg &message)
{
    return trySendFor(time - TickTimer::now(), message);
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

template <typename Msg> auto Queue<Msg>::tryFrontSendUntil(const TickTimer::TimePoint &time, const Msg &message)
{
    return tryFrontSendFor(time - TickTimer::now(), message);
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
