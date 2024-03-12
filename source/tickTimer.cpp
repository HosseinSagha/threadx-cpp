#include "tickTimer.hpp"
#include <utility>

namespace ThreadX
{
TickTimer::TickTimer(const Duration &timeout, const ExpirationCallback &expirationCallback, const TimerType type,
                     const ActivationType activationType)
    : Native::TX_TIMER{}, m_timeout{timeout}, m_expirationCallback{expirationCallback},
      m_id{expirationCallback ? ++m_idCounter : 0}, m_type{type}
{
    using namespace Native;
    [[maybe_unused]] Error error{tx_timer_create(
        this, const_cast<char *>("timer"), m_expirationCallback ? TickTimer::expirationCallback : nullptr,
        reinterpret_cast<Ulong>(this), ticks(timeout), type == TimerType::SingleShot ? 0 : ticks(timeout),
        std::to_underlying(activationType))};
    assert(error == Error::success);
}

TickTimer::~TickTimer()
{
    tx_timer_delete(this);
}

void TickTimer::now(const TimePoint &time)
{
    Native::tx_time_set(ticks(time.time_since_epoch()));
}

TickTimer::TimePoint TickTimer::now()
{
    return TimePoint{Duration{Native::tx_time_get()}};
}

std::time_t TickTimer::to_time_t(const TimePoint &time)
{
    return duration_cast<std::chrono::seconds>(time.time_since_epoch()).count();
}

TickTimer::TimePoint TickTimer::from_time_t(const std::time_t &time)
{
    using namespace std::chrono;
    return time_point_cast<Duration>(std::chrono::time_point<TickTimer, seconds>(seconds{time}));
}

Error TickTimer::activate()
{
    return Error{tx_timer_activate(this)};
}

Error TickTimer::deactivate()
{
    return Error{tx_timer_deactivate(this)};
}

Error TickTimer::change(const Duration &timeout, const ActivationType activationType)
{
    return change(timeout, m_type, activationType);
}

Error TickTimer::change(const Duration &timeout, const TimerType type, const ActivationType activationType)
{
    Error error{deactivate()};
    assert(error == Error::success);

    error = Error{tx_timer_change(this, ticks(timeout), type == TimerType::SingleShot ? 0 : ticks(timeout))};

    m_timeout = timeout;
    m_type = type;

    if (activationType == ActivationType::autoActivate)
    {
        activate();
    }

    return error;
}

Error TickTimer::reactivate()
{
    return change(m_timeout);
}

size_t TickTimer::id() const
{
    return m_id;
}

void TickTimer::expirationCallback(const Ulong timerPtr)
{
    auto &timer{*reinterpret_cast<TickTimer *>(timerPtr)};
    timer.m_expirationCallback(timer.m_id);
}
} // namespace ThreadX
