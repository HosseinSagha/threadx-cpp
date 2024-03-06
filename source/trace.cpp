#ifdef TX_ENABLE_EVENT_TRACE

#include "trace.hpp"

namespace ThreadX
{
Error TraceBase::eventFilter(const Ulong eventBits)
{
    return Error{Native::tx_trace_event_filter(eventBits)};
}

Error TraceBase::eventUnfilter(const Ulong eventBits)
{
    return Error{Native::tx_trace_event_unfilter(eventBits)};
}

Error TraceBase::eventFilter(const TraceEvent event)
{
    return Error{Native::tx_trace_event_filter(static_cast<Ulong>(event))};
}

Error TraceBase::eventUnfilter(const TraceEvent event)
{
    return Error{Native::tx_trace_event_unfilter(static_cast<Ulong>(event))};
}

Error TraceBase::disable()
{
    return Error{Native::tx_trace_disable()};
}

void TraceBase::isrEnterInsert(const Ulong isrID)
{
    Native::tx_trace_isr_enter_insert(isrID);
}

void TraceBase::isrExitInsert(const Ulong isrID)
{
    Native::tx_trace_isr_exit_insert(isrID);
}

Error TraceBase::userEventInsert(
    const Ulong eventID, const Ulong infoField1, const Ulong infoField2, const Ulong infoField3, const Ulong infoField4)
{
    assert(eventID >= TX_TRACE_USER_EVENT_START and eventID <= TX_TRACE_USER_EVENT_END);
    return Error{Native::tx_trace_user_event_insert(eventID, infoField1, infoField2, infoField3, infoField4)};
}
} // namespace ThreadX
#endif