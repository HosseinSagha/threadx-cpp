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
    [[maybe_unused]] Error error{tx_event_flags_delete(this)};
    assert(error == Error::success);
}

Error EventFlags::set(const Bitmask &bitMask)
{
    return Error{tx_event_flags_set(this, bitMask.to_ulong(), std::to_underlying(FlagOption::orInto))};
}

Error EventFlags::clear(const Bitmask &bitMask)
{
    return Error{tx_event_flags_set(this, (~bitMask).to_ulong(), std::to_underlying(FlagOption::andInto))};
}

EventFlags::ExpectedBitmask EventFlags::get(const Bitmask &bitMask, const Option option)
{
    return waitAllFor(bitMask, TickTimer::noWait, option);
}

EventFlags::ExpectedBitmask EventFlags::waitAll(const Bitmask &bitMask, const Option option)
{
    return waitAllFor(bitMask, TickTimer::waitForever, option);
}

EventFlags::ExpectedBitmask EventFlags::waitAny(const Bitmask &bitMask, const Option option)
{
    return waitAnyFor(bitMask, TickTimer::waitForever, option);
}

std::string_view EventFlags::name() const
{
    return tx_event_flags_group_name;
}

void EventFlags::setNotifyCallback(Native::TX_EVENT_FLAGS_GROUP *notifyGroupPtr)
{
    auto &eventFlags{static_cast<EventFlags &>(*notifyGroupPtr)};
    eventFlags.m_setNotifyCallback(eventFlags);
}
} // namespace ThreadX
