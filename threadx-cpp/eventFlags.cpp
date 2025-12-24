#include "eventFlags.hpp"
#include <utility>

namespace ThreadX
{
EventFlags::EventFlags(const std::string_view name, const NotifyCallback setNotifyCallback)
    : Native::TX_EVENT_FLAGS_GROUP{}, m_setNotifyCallback{std::move(setNotifyCallback)}
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

auto EventFlags::set(const Bitmask &bitMask) -> Error
{
    return Error{tx_event_flags_set(this, bitMask.to_ulong(), std::to_underlying(FlagOption::orInto))};
}

auto EventFlags::clear(const Bitmask &bitMask) -> Error
{
    return Error{tx_event_flags_set(this, (~bitMask).to_ulong(), std::to_underlying(FlagOption::andInto))};
}

auto EventFlags::get(const Option option) -> ExpectedBitmask
{
    return get(allBits, option);
}

auto EventFlags::get(const Bitmask &bitMask, const Option option) -> ExpectedBitmask
{
    if (auto expectedBitmask{waitAllFor(bitMask, TickTimer::noWait, option)}; expectedBitmask or expectedBitmask.error() != Error::noEvents)
    {
        return expectedBitmask;
    }

    return Bitmask{0UL};
}

auto EventFlags::waitAll(const Bitmask &bitMask, const Option option) -> ExpectedBitmask
{
    return waitAllFor(bitMask, TickTimer::waitForever, option);
}

auto EventFlags::waitAny(const Bitmask &bitMask, const Option option) -> ExpectedBitmask
{
    return waitAnyFor(bitMask, TickTimer::waitForever, option);
}

[[gnu::pure]] auto EventFlags::name() const -> std::string_view
{
    return tx_event_flags_group_name;
}

auto EventFlags::setNotifyCallback(Native::TX_EVENT_FLAGS_GROUP *notifyGroupPtr) -> void
{
    auto &eventFlags{static_cast<EventFlags &>(*notifyGroupPtr)};
    eventFlags.m_setNotifyCallback(eventFlags);
}
} // namespace ThreadX
