#pragma once

#include "txCommon.hpp"
#include <chrono>
#include <functional>

namespace ThreadX
{
///
enum class TimerType
{
    Continuous, ///< Continuous
    SingleShot  ///< SingleShot
};
///
enum class ActivationType : Uint
{
    noActivate,  ///< noActivate Not active after creation.
    autoActivate ///< autoActivate Activate on creation.
};

///
class TickTimer : Native::TX_TIMER
{
  public:
    using ExpirationCallback = std::function<void(Ulong)>;
    using rep = double;
    using period = std::ratio<1, TX_TIMER_TICKS_PER_SECOND>;
    using duration = std::chrono::duration<rep, period>;
    using Duration = duration;
    using time_point = std::chrono::time_point<TickTimer, Duration>;
    using TimePoint = time_point;
    static constexpr bool is_steady = true;

    static constexpr Duration noWait{};
    static constexpr Duration waitForever{0xFFFFFFFFUL};

    /// Constructor
    // ID zero means no callback and therefore passed callbackID never matches timer objects with no callback
    /// \param timeout
    /// \param expirationCallback function to call when timeout happens.
    /// Use CALLLBACK_BIND to pass callback as any other object's member function.
    /// \param type \sa TimerType
    /// \param activationType \sa ActivationType
    TickTimer(const Duration &timeout, const ExpirationCallback &expirationCallback = {},
              const TimerType type = TimerType::Continuous,
              const ActivationType activationType = ActivationType::autoActivate);

    /// Destructor. deletes the timer.
    ~TickTimer();

    static constexpr auto ticks(const Duration &duration);
    static constexpr auto ticks(const TimePoint &time);

    /// sets the internal system clock to the specified value.
    /// \param time
    static void now(const TimePoint &time);

    /// returns the internal system clock.
    static TimePoint now();

    /// activates the specified application timer
    Error activate();

    /// Deactivate application timer
    Error deactivate();

    Error change(const Duration &timeout, ActivationType activationType = ActivationType::autoActivate);

    /// change timeout or type. The timer must be deactivated prior to calling this service, and activated afterwards.
    /// An expired one-shot timer must be reset via change() before it can be activated again.
    /// \param timeout
    /// \param type
    /// \param activationType
    Error change(const Duration &timeout, const TimerType type,
                 const ActivationType activationType = ActivationType::autoActivate);

    Error reactivate();

    size_t id() const;

  private:
    static void expirationCallback(Ulong timerPtr);

    static inline size_t m_idCounter;
    Duration m_timeout;
    const ExpirationCallback m_expirationCallback;
    const size_t m_id;
    TimerType m_type;
};

static_assert(std::chrono::is_clock_v<TickTimer>);

constexpr auto TickTimer::ticks(const Duration &duration)
{
    return Ulong(duration.count());
}

constexpr auto TickTimer::ticks(const TimePoint &time)
{
    return ticks(time.time_since_epoch());
}
} // namespace ThreadX
