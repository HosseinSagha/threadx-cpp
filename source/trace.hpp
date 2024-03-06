#pragma once

#ifdef TX_ENABLE_EVENT_TRACE

#include "fxCommon.hpp"
#include "txCommon.hpp"
#include <array>

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

inline Ulong operator|(const TraceEvent eventType1, const TraceEvent eventType2)
{
    return std::to_underlying(eventType1) | std::to_underlying(eventType2);
}

inline Ulong operator|(const Ulong eventType1, const TraceEvent eventType2)
{
    return eventType1 | std::to_underlying(eventType2);
}

class TraceBase
{
  public:
    TraceBase &operator=(const TraceBase &) = delete;
    TraceBase(const TraceBase &) = delete;

    static Error eventFilter(const Ulong eventBits);

    static Error eventUnfilter(const Ulong eventBits);

    static Error eventFilter(const TraceEvent event);

    static Error eventUnfilter(const TraceEvent event);

    static Error disable();

    static void isrEnterInsert(const Ulong isrID);

    static void isrExitInsert(const Ulong isrID);

    static Error userEventInsert(const Ulong eventID, const Ulong infoField1, const Ulong infoField2,
                                 const Ulong infoField3, const Ulong infoField4);

  protected:
    TraceBase() = default;
};

///
template <Ulong Size> class Trace : public TraceBase, std::array<std::byte, Size>
{
  public:
    using Callback = void (*)(void *);

    ///
    /// \param registryEntries
    /// \param bufferFullNotifyCallback passed pointer to callback is TX_TRACE_HEADER pointer.
    /// \return
    Trace(const Ulong registryEntries, const Callback bufferFullNotifyCallback = {});
};

template <Ulong Size> Trace<Size>::Trace(const Ulong registryEntries, const Callback bufferFullNotifyCallback)
{
    [[maybe_unused]] Error error{Native::tx_trace_enable(this->data(), Size, registryEntries)};
    assert(error == Error::success);

    if (bufferFullNotifyCallback)
    {
        error = Error{Native::tx_trace_buffer_full_notify(bufferFullNotifyCallback)};
        assert(error == Error::success);
    }
}
} // namespace ThreadX
#endif