#include "tickTimer.hpp"
#include <cassert>
#include <string_view>

namespace ThreadX
{
auto TickTimer::now() -> TimePoint
{
    return TimePoint{Duration{Native::tx_time_get()}};
}

TickTimer::~TickTimer()
{
    [[maybe_unused]] Error error{tx_timer_delete(this)};
    assert(error == Error::success);
}

auto TickTimer::activate() -> Error
{
    return Error{tx_timer_activate(this)};
}

auto TickTimer::deactivate() -> Error
{
    return Error{tx_timer_deactivate(this)};
}

auto TickTimer::reset() -> Error
{
    return reset(Duration{m_timeoutTicks}, m_type, m_activationType);
}

[[gnu::pure]] auto TickTimer::id() const -> size_t
{
    return m_id;
}

[[gnu::pure]] auto TickTimer::name() const -> std::string_view
{
    return std::string_view{tx_timer_name};
}

auto TickTimer::expirationCallback(const Ulong timerPtr) -> void
{
    auto &timer{*reinterpret_cast<TickTimer *>(timerPtr)};
    timer.m_expirationCallback(timer.m_id);
}
} // namespace ThreadX
