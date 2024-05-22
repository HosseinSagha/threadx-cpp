#include "tickTimer.hpp"

namespace ThreadX
{
void TickTimer::now(const TimePoint &time)
{
    Native::tx_time_set(ticks(time.time_since_epoch()));
}

TickTimer::TimePoint TickTimer::now()
{
    return TimePoint{Duration{Native::tx_time_get()}};
}

TickTimer::TimePair TickTimer::to_time_t(const TimePoint &time)
{
    using namespace std::chrono;
    auto frac_ms{time_point_cast<milliseconds>(time) - time_point_cast<milliseconds>(time_point_cast<seconds>(time))};
    return {duration_cast<seconds>(time.time_since_epoch()).count(), frac_ms.count()};
}

TickTimer::TimePoint TickTimer::from_time_t(const std::time_t &time)
{
    using namespace std::chrono;
    return time_point_cast<Duration>(std::chrono::time_point<TickTimer, seconds>(seconds{time}));
}

TickTimer::TmPair TickTimer::to_localtime(const TimePoint &time)
{
    auto [t, frac_ms]{to_time_t(time)};
    return {*std::localtime(std::addressof(t)), frac_ms};
}

TickTimer::TimePoint TickTimer::from_localtime(const std::tm &localtime)
{
    return from_time_t(mktime(const_cast<std::tm *>(std::addressof(localtime))));
}

TickTimer::~TickTimer()
{
    tx_timer_delete(this);
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

Error TickTimer::change(const Duration &timeout, const Type type, const ActivationType activationType)
{
    Error error{deactivate()};
    assert(error == Error::success);

    error = Error{tx_timer_change(this, ticks(timeout), type == Type::SingleShot ? 0 : ticks(timeout))};

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

std::string_view TickTimer::name() const
{
    return std::string_view(tx_timer_name);
}

void TickTimer::expirationCallback(const Ulong timerPtr)
{
    auto &timer{*reinterpret_cast<TickTimer *>(timerPtr)};
    timer.m_expirationCallback(timer.m_id);
}
} // namespace ThreadX
