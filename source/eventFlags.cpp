#include "eventFlags.hpp"
#include <utility>

namespace ThreadX
{
EventFlags::EventFlags(const std::string_view name, const NotifyCallback &setNotifyCallback)
    : Native::TX_EVENT_FLAGS_GROUP{}, m_setNotifyCallback{setNotifyCallback}
{
    using namespace Native;
    [[maybe_unused]] Error error{tx_event_flags_create(this, const_cast<char *>(name.data()))};
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

Error EventFlags::set(const Bitmask &bitMask)
{
    return Error{tx_event_flags_set(this, bitMask.to_ulong(), std::to_underlying(Option::orInto))};
}

Error EventFlags::clear(const Bitmask &bitMask)
{
    return Error{tx_event_flags_set(this, (~bitMask).to_ulong(), std::to_underlying(Option::andInto))};
}

EventFlags::BitmaskPair EventFlags::get(const Bitmask &bitMask, const EventOption eventOption)
{
    return waitAllFor(bitMask, TickTimer::noWait, eventOption);
}

EventFlags::BitmaskPair EventFlags::waitAll(const Bitmask &bitMask, const EventOption eventOption)
{
    return waitAllFor(bitMask, TickTimer::waitForever, eventOption);
}

EventFlags::BitmaskPair EventFlags::waitAny(const Bitmask &bitMask, const EventOption eventOption)
{
    return waitAnyFor(bitMask, TickTimer::waitForever, eventOption);
}

EventFlags::BitmaskPair EventFlags::waitFor(
    const Bitmask &bitMask, const TickTimer::Duration &waitDuration, const Option option)
{
    Ulong actualFlags{};
    Error error{tx_event_flags_get(this, bitMask.to_ulong(), std::to_underlying(option), std::addressof(actualFlags),
                                   TickTimer::ticks(waitDuration))};

    return {error, Bitmask{actualFlags}};
}

std::string_view EventFlags::name()
{
    return tx_event_flags_group_name;
}

void EventFlags::setNotifyCallback(auto notifyGroupPtr)
{
    auto &eventFlags{static_cast<EventFlags &>(*notifyGroupPtr)};
    eventFlags.m_setNotifyCallback(eventFlags);
}
} // namespace ThreadX
