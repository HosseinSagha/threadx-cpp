#pragma once

#ifdef TX_ENABLE_EVENT_TRACE

#include "fxCommon.hpp"
#include "txCommon.hpp"
#include <array>
#include <type_traits> // for std::to_underlying

namespace ThreadX
{
enum class TraceEvent : Ulong
{
    txAllEvents = TX_TRACE_ALL_EVENTS,
    txInternalEvents = TX_TRACE_INTERNAL_EVENTS,
    txBlockPoolEvents = TX_TRACE_BLOCK_POOL_EVENTS,
    txBytePoolEvents = TX_TRACE_BYTE_POOL_EVENTS,
    txEventFlagsEvents = TX_TRACE_EVENT_FLAGS_EVENTS,
    txInterruptControlEvent = TX_TRACE_INTERRUPT_CONTROL_EVENT,
    txMutexEvents = TX_TRACE_MUTEX_EVENTS,
    txQueueEvents = TX_TRACE_QUEUE_EVENTS,
    txSemaphoreEvents = TX_TRACE_SEMAPHORE_EVENTS,
    txThreadEvents = TX_TRACE_THREAD_EVENTS,
    txTimeEvents = TX_TRACE_TIME_EVENTS,
    txTimerEvents = TX_TRACE_TIMER_EVENTS,
    fxAllEvents = FX_TRACE_ALL_EVENTS,
    fxInternalEvents = FX_TRACE_INTERNAL_EVENTS,
    fxMediaEvents = FX_TRACE_MEDIA_EVENTS,
    fxDirectoryEvents = FX_TRACE_DIRECTORY_EVENTS,
    fxFileEvents = FX_TRACE_FILE_EVENTS,
#if 0
    nxAllEvents = NX_TRACE_ALL_EVENTS,
    nxInternalEvents = NX_TRACE_INTERNAL_EVENTS,
    nxArpEvents = NX_TRACE_ARP_EVENTS,
    nxIcmpEvents = NX_TRACE_ICMP_EVENTS,
    nxIgmpEvents = NX_TRACE_IGMP_EVENTS,
    nxIPEvents = NX_TRACE_IP_EVENTS,
    nxPacketEvents = NX_TRACE_PACKET_EVENTS,
    nxRarpEvents = NX_TRACE_RARP_EVENTS,
    nxTcpEvents = NX_TRACE_TCP_EVENTS,
    nxUdpEvents = NX_TRACE_UDP_EVENTS,
    uxAllEvents = UX_TRACE_ALL_EVENTS,
    uxErrors = UX_TRACE_ERRORS,
    uxHostStackEvents = UX_TRACE_HOST_STACK_EVENTS,
    uxDeviceStackEvents = UX_TRACE_DEVICE_STACK_EVENTS,
    uxHostControllerEvents = UX_TRACE_HOST_CONTROLLER_EVENTS,
    uxDeviceControllerEvents = UX_TRACE_DEVICE_CONTROLLER_EVENTS,
    uxHostClassEvents = UX_TRACE_HOST_CLASS_EVENTS,
    uxTraceDeviceClassEvents = UX_TRACE_DEVICE_CLASS_EVENTS
#endif
};

inline auto operator|(const TraceEvent eventType1, const TraceEvent eventType2) -> Ulong
{
    return std::to_underlying(eventType1) | std::to_underlying(eventType2);
}

inline auto operator|(const Ulong eventType1, const TraceEvent eventType2) -> Ulong
{
    return eventType1 | std::to_underlying(eventType2);
}

///
template <Ulong Size>
class Trace
{
  public:
    Trace &operator=(const Trace &) = delete;
    Trace(const Trace &) = delete;

    ///
    /// \param registryEntries
    /// \param bufferFullNotifyCallback passed pointer to callback is TX_TRACE_HEADER pointer.
    /// \return
    explicit Trace(const Ulong registryEntries);

    static auto eventFilter(const Ulong eventBits) -> Error;

    static auto eventUnfilter(const Ulong eventBits) -> Error;

    static auto eventFilter(const TraceEvent event) -> Error;

    static auto eventUnfilter(const TraceEvent event) -> Error;

    static auto disable() -> Error;

    static auto isrEnterInsert(const Ulong isrID) -> void;

    static auto isrExitInsert(const Ulong isrID) -> void;

    static auto userEventInsert(const Ulong eventID, const Ulong infoField1, const Ulong infoField2, const Ulong infoField3, const Ulong infoField4) -> Error;

  private:
    std::array<Uchar, Size> m_trace{};
};

template <Ulong Size>
Trace<Size>::Trace(const Ulong registryEntries)
{
    [[maybe_unused]] Error error{Native::tx_trace_enable(m_trace.data(), Size, registryEntries)};
    assert(error == Error::success);
}

template <Ulong Size>
auto Trace<Size>::eventFilter(const Ulong eventBits) -> Error
{
    return Error{Native::tx_trace_event_filter(eventBits)};
}

template <Ulong Size>
auto Trace<Size>::eventUnfilter(const Ulong eventBits) -> Error
{
    return Error{Native::tx_trace_event_unfilter(eventBits)};
}

template <Ulong Size>
auto Trace<Size>::eventFilter(const TraceEvent event) -> Error
{
    return Error{Native::tx_trace_event_filter(std::to_underlying(event))};
}

template <Ulong Size>
auto Trace<Size>::eventUnfilter(const TraceEvent event) -> Error
{
    return Error{Native::tx_trace_event_unfilter(std::to_underlying(event))};
}

template <Ulong Size>
auto Trace<Size>::disable() -> Error
{
    return Error{Native::tx_trace_disable()};
}

template <Ulong Size>
auto Trace<Size>::isrEnterInsert(const Ulong isrID) -> void
{
    Native::tx_trace_isr_enter_insert(isrID);
}

template <Ulong Size>
auto Trace<Size>::isrExitInsert(const Ulong isrID) -> void
{
    Native::tx_trace_isr_exit_insert(isrID);
}

template <Ulong Size>
auto Trace<Size>::userEventInsert(const Ulong eventID, const Ulong infoField1, const Ulong infoField2, const Ulong infoField3, const Ulong infoField4) -> Error
{
    assert(eventID >= TX_TRACE_USER_EVENT_START and eventID <= TX_TRACE_USER_EVENT_END);
    return Error{Native::tx_trace_user_event_insert(eventID, infoField1, infoField2, infoField3, infoField4)};
}

using TraceBufFullNotifyCallback = void (*)(void *);

inline auto registerbufFullNotifyCallback(const TraceBufFullNotifyCallback bufferFullNotifyCallback) -> Error
{
    return Error{Native::tx_trace_buffer_full_notify(bufferFullNotifyCallback)};
}
} // namespace ThreadX
#endif // TX_ENABLE_EVENT_TRACE
