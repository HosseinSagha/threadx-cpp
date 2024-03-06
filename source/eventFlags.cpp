#include "eventFlags.hpp"
#include <utility>

namespace ThreadX
{
EventFlags::EventFlags(const NotifyCallback &setNotifyCallback)
    : Native::TX_EVENT_FLAGS_GROUP{}, m_setNotifyCallback{setNotifyCallback}
{
    using namespace Native;
    [[maybe_unused]] Error error{tx_event_flags_create(this, const_cast<char *>("event flags"))};
    assert(error == Error::success);

    if (m_setNotifyCallback)
    {
        error = Error{tx_event_flags_set_notify(this, EventFlags::setNotifyCallback)};
        assert(error == Error::success);
    }
}

EventFlags::~EventFlags()
{
    tx_event_flags_delete(this);
}

Error EventFlags::set(const BitMask &bitMask)
{
    return Error{tx_event_flags_set(this, bitMask.to_ulong(), std::to_underlying(Option::orInto))};
}

Error EventFlags::clear(const BitMask &bitMask)
{
    return Error{tx_event_flags_set(this, (~bitMask).to_ulong(), std::to_underlying(Option::andInto))};
}

EventFlags::ReturnTuple EventFlags::get(const BitMask &bitMask, const EventOption eventOption)
{
    return waitAllFor(bitMask, TickTimer::noWait, eventOption);
}

EventFlags::ReturnTuple EventFlags::waitAll(const BitMask &bitMask, const EventOption eventOption)
{
    return waitAllFor(bitMask, TickTimer::waitForever, eventOption);
}

EventFlags::ReturnTuple EventFlags::waitAllUntil(
    const BitMask &bitMask, const TickTimer::TimePoint &timePoint, const EventOption eventOption)
{
    return waitAllFor(bitMask, timePoint - TickTimer::now(), eventOption);
}

EventFlags::ReturnTuple EventFlags::waitAllFor(
    const BitMask &bitMask, const TickTimer::Duration &waitDuration, const EventOption eventOption)
{
    auto option{Option::allClear};
    if (eventOption == EventOption::dontClear)
    {
        option = Option::all;
    }

    return waitFor(bitMask, waitDuration, option);
}

EventFlags::ReturnTuple EventFlags::waitAny(const BitMask &bitMask, const EventOption eventOption)
{
    return waitAnyFor(bitMask, TickTimer::waitForever, eventOption);
}

EventFlags::ReturnTuple EventFlags::waitAnyUntil(
    const BitMask &bitMask, const TickTimer::TimePoint &timePoint, const EventOption eventOption)
{
    return waitAnyFor(bitMask, timePoint - TickTimer::now(), eventOption);
}

EventFlags::ReturnTuple EventFlags::waitAnyFor(
    const BitMask &bitMask, const TickTimer::Duration &waitDuration, const EventOption eventOption)
{
    auto option{Option::anyClear};
    if (eventOption == EventOption::dontClear)
    {
        option = Option::any;
    }

    return waitFor(bitMask, waitDuration, option);
}

EventFlags::ReturnTuple EventFlags::waitFor(
    const BitMask &bitMask, const TickTimer::Duration &waitDuration, const Option option)
{
    Ulong actualFlags{};
    Error error{tx_event_flags_get(this, bitMask.to_ulong(), std::to_underlying(option), std::addressof(actualFlags),
                                   TickTimer::ticks(waitDuration))};

    return {error, BitMask{actualFlags}};
}

void EventFlags::setNotifyCallback(auto notifyGroupPtr)
{
    auto &eventFlags{static_cast<EventFlags &>(*notifyGroupPtr)};
    eventFlags.m_setNotifyCallback(eventFlags);
}
} // namespace ThreadX
