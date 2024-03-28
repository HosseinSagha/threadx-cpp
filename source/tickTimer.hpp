#pragma once

#include "txCommon.hpp"
#include <chrono>
#include <functional>
#include <utility>

namespace ThreadX
{
///
class TickTimer : Native::TX_TIMER
{
  public:
    ///
    enum class Type
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

    using ExpirationCallback = std::function<void(Ulong)>;
    using rep = Ulong;
    using period = std::ratio<1, TX_TIMER_TICKS_PER_SECOND>;
    using duration = std::chrono::duration<rep, period>;
    using Duration = duration;
    using time_point = std::chrono::time_point<TickTimer, Duration>;
    using TimePoint = time_point;
    using TimePair = std::pair<time_t, Ulong>;
    using TmPair = std::pair<std::tm, Ulong>;

    static constexpr bool is_steady = true;
    static constexpr Duration noWait{};
    static constexpr Duration waitForever{0xFFFFFFFFUL};

    static constexpr auto ticks(const Duration &duration);
    static constexpr auto ticks(const TimePoint &time);

    /// sets the internal system clock to the specified value.
    /// \param time
    static void now(const TimePoint &time);

    /// returns the internal system clock.
    static TimePoint now();

    static TimePair to_time_t(const TimePoint &time);

    static TimePoint from_time_t(const std::time_t &time);

    static TmPair to_localtime(const TimePoint &time);

    static TimePoint from_localtime(const std::tm &localtime);

    /// Constructor
    // ID zero means no callback and therefore passed callbackID never matches timer objects with no callback
    /// \param timeout
    /// \param expirationCallback function to call when timeout happens.
    /// Use CALLLBACK_BIND to pass callback as any other object's member function.
    /// \param type \sa Type
    /// \param activationType \sa ActivationType
    template <typename Rep, typename Period>
    TickTimer(const std::chrono::duration<Rep, Period> &timeout, const ExpirationCallback &expirationCallback = {},
              const Type type = Type::Continuous, const ActivationType activationType = ActivationType::autoActivate);

    /// Destructor. deletes the timer.
    ~TickTimer();

    /// activates the specified application timer
    Error activate();

    /// Deactivate application timer
    Error deactivate();

    Error change(const Duration &timeout, const ActivationType activationType = ActivationType::autoActivate);

    /// change timeout or type. The timer must be deactivated prior to calling this service, and activated afterwards.
    /// An expired one-shot timer must be reset via change() before it can be activated again.
    /// \param timeout
    /// \param type
    /// \param activationType
    Error change(
        const Duration &timeout, const Type type, const ActivationType activationType = ActivationType::autoActivate);

    Error reactivate();

    size_t id() const;

  private:
    static void expirationCallback(const Ulong timerPtr);

    static inline size_t m_idCounter;
    Duration m_timeout;
    const ExpirationCallback m_expirationCallback;
    const size_t m_id;
    Type m_type;
};

static_assert(std::chrono::is_clock_v<TickTimer>);

constexpr auto TickTimer::ticks(const Duration &duration)
{
    return duration.count();
}

constexpr auto TickTimer::ticks(const TimePoint &time)
{
    return ticks(time.time_since_epoch());
}

template <typename Rep, typename Period>
TickTimer::TickTimer(const std::chrono::duration<Rep, Period> &timeout, const ExpirationCallback &expirationCallback,
                     const Type type, const ActivationType activationType)
    : Native::TX_TIMER{}, m_timeout{std::chrono::duration_cast<TickTimer::Duration>(timeout)},
      m_expirationCallback{expirationCallback}, m_id{expirationCallback ? ++m_idCounter : 0}, m_type{type}
{
    using namespace Native;
    [[maybe_unused]] Error error{tx_timer_create(
        this, const_cast<char *>("tick timer"), m_expirationCallback ? TickTimer::expirationCallback : nullptr,
        reinterpret_cast<Ulong>(this), ticks(std::chrono::duration_cast<TickTimer::Duration>(timeout)),
        type == Type::SingleShot ? 0 : ticks(std::chrono::duration_cast<TickTimer::Duration>(timeout)),
        std::to_underlying(activationType))};
    assert(error == Error::success);
}
} // namespace ThreadX
