#pragma once

#include "txCommon.hpp"
#include <atomic>
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
    static constexpr Duration noWait{0UL};
    static constexpr Duration waitForever{0xFFFFFFFFUL};

    static constexpr auto ticks(const auto &duration);

    /// returns the internal tick count.
    static TimePoint now();

    /// Constructor
    // ID zero means no callback and therefore passed callbackID never matches timer objects with no callback
    /// \param timeout
    /// \param expirationCallback function to call when timeout happens.
    /// Use CALLLBACK_BIND to pass callback as any other object's member function.
    /// \param type \sa Type
    /// \param activationType \sa ActivationType
    explicit TickTimer(
        const std::string_view name, const auto &timeout, const ExpirationCallback &expirationCallback = {},
        const Type type = Type::Continuous, const ActivationType activationType = ActivationType::autoActivate);

    /// Destructor. deletes the timer.
    ~TickTimer();

    /// activates the specified application timer
    Error activate();

    /// Deactivate application timer
    Error deactivate();

    auto change(const auto &timeout, const ActivationType activationType = ActivationType::autoActivate);

    /// change timeout or type. The timer must be deactivated prior to calling this service, and activated afterwards.
    /// An expired one-shot timer must be reset via change() before it can be activated again.
    /// \param timeout
    /// \param type
    /// \param activationType
    auto change(
        const auto &timeout, const Type type, const ActivationType activationType = ActivationType::autoActivate);

    Error reactivate();

    size_t id() const;

    std::string_view name() const;

  private:
    static void expirationCallback(const Ulong timerPtr);

    static inline std::atomic_size_t m_idCounter{1}; //id=0 is reserved for timers with no callback
    Duration m_timeout;
    const ExpirationCallback m_expirationCallback;
    const size_t m_id;
    Type m_type;
};

static_assert(std::chrono::is_clock_v<TickTimer>);

constexpr auto TickTimer::ticks(const auto &duration)
{
    if (duration.count() < 0)
    {
        return 0UL;
    }

    return std::chrono::duration_cast<TickTimer::Duration>(duration).count();
}

TickTimer::TickTimer(const std::string_view name, const auto &timeout, const ExpirationCallback &expirationCallback,
                     const Type type, const ActivationType activationType)
    : Native::TX_TIMER{}, m_timeout{std::chrono::duration_cast<TickTimer::Duration>(timeout)},
      m_expirationCallback{expirationCallback}, m_id{expirationCallback ? m_idCounter.fetch_add(1) : 0}, m_type{type}
{
    using namespace Native;
    [[maybe_unused]] Error error{tx_timer_create(
        this, const_cast<char *>(name.data()), m_expirationCallback ? TickTimer::expirationCallback : nullptr,
        reinterpret_cast<Ulong>(this), ticks(timeout), type == Type::SingleShot ? 0 : ticks(timeout),
        std::to_underlying(activationType))};

    assert(error == Error::success);
}

auto TickTimer::change(const auto &timeout, const ActivationType activationType)
{
    return change(timeout, m_type, activationType);
}

auto TickTimer::change(const auto &timeout, const Type type, const ActivationType activationType)
{
    Error error{deactivate()};
    if (error != Error::success)
    {
        return error;
    }

    error = Error{tx_timer_change(this, ticks(timeout), type == Type::SingleShot ? 0 : ticks(timeout))};
    if (error != Error::success)
    {
        return error;
    }

    m_timeout = std::chrono::duration_cast<TickTimer::Duration>(timeout);
    m_type = type;

    if (activationType == ActivationType::autoActivate)
    {
        activate();
    }

    return Error::success;
}
} // namespace ThreadX
