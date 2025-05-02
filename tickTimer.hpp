#pragma once

#include "txCommon.hpp"
#include <atomic>
#include <chrono>
#include <functional>
#include <string_view>
#include <utility>

namespace ThreadX
{
///
class TickTimer final : Native::TX_TIMER
{
  public:
    ///
    enum class Type
    {
        periodic, ///< periodic
        periodicImmediate,
        oneShot ///< oneShot
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

    static constexpr bool is_steady = true;
    static constexpr Duration noWait{0UL};
    static constexpr Duration waitForever{0xFFFFFFFFUL};

    template <typename Rep, typename Period>
    static constexpr auto ticks(const std::chrono::duration<Rep, Period> &duration);

    /// returns the internal tick count.
    static TimePoint now();

    /// Constructor
    // ID zero means no callback and therefore passed callbackID never matches timer objects with no callback
    /// \param timeout
    /// \param expirationCallback function to call when timeout happens.
    /// Use CALLLBACK_BIND to pass callback as any other object's member function.
    /// \param type \sa Type
    /// \param activationType \sa ActivationType
    explicit TickTimer(const std::string_view name, const auto &timeout, const ExpirationCallback &expirationCallback = {}, const Type type = Type::periodic,
                       const ActivationType activationType = ActivationType::autoActivate);

    /// Destructor. deletes the timer.
    ~TickTimer();

    Error reset();
    template <typename Rep, typename Period>
    auto reset(const std::chrono::duration<Rep, Period> &timeout);
    template <typename Rep, typename Period>
    auto reset(const std::chrono::duration<Rep, Period> &timeout, const Type type);
    template <typename Rep, typename Period>
    auto reset(const std::chrono::duration<Rep, Period> &timeout, const ActivationType activationType);
    /// change timeout or type. The timer must be deactivated prior to calling this service, and activated afterwards.
    /// An expired one-shot timer must be reset via change() before it can be activated again.
    /// \param timeout
    /// \param type
    /// \param activationType
    template <typename Rep, typename Period>
    auto reset(const std::chrono::duration<Rep, Period> &timeout, const Type type, const ActivationType activationType);

    /// activates the specified application timer
    Error activate();
    /// Deactivate application timer
    Error deactivate();
    size_t id() const;
    std::string_view name() const;

  private:
    static void expirationCallback(const Ulong timerPtr);

    static inline std::atomic_size_t m_idCounter{}; // id=0 is reserved for timers with no callback
    rep m_timeoutTicks;
    const ExpirationCallback m_expirationCallback;
    const size_t m_id;
    Type m_type;
    ActivationType m_activationType;
};

static_assert(std::chrono::is_clock_v<TickTimer>);

/// Returns the internal tick count ceiled to tick duration (usually 10ms).
///\tparam Rep
///\tparam Period
///\param duration
///\return constexpr auto
template <typename Rep, typename Period>
constexpr auto TickTimer::ticks(const std::chrono::duration<Rep, Period> &duration)
{
    if (duration.count() < 0)
    {
        return 0UL;
    }

    return std::chrono::ceil<TickTimer::Duration>(duration).count();
}

TickTimer::TickTimer(const std::string_view name, const auto &timeout, const ExpirationCallback &expirationCallback, const Type type,
                     const ActivationType activationType)
    : Native::TX_TIMER{}, m_timeoutTicks{ticks(timeout)}, m_expirationCallback{expirationCallback}, m_id{expirationCallback ? ++m_idCounter : 0}, m_type{type},
      m_activationType{activationType}
{
    using namespace Native;
    [[maybe_unused]] Error error{tx_timer_create(this, const_cast<char *>(name.data()), m_expirationCallback ? TickTimer::expirationCallback : nullptr,
                                                 reinterpret_cast<Ulong>(this), type == Type::periodicImmediate ? 1UL : m_timeoutTicks,
                                                 type == Type::oneShot ? 0UL : m_timeoutTicks, std::to_underlying(activationType))};

    assert(error == Error::success);
}

template <typename Rep, typename Period>
auto TickTimer::reset(const std::chrono::duration<Rep, Period> &timeout)
{
    return reset(timeout, m_type, m_activationType);
}

template <typename Rep, typename Period>
auto TickTimer::reset(const std::chrono::duration<Rep, Period> &timeout, const Type type)
{
    return reset(timeout, type, m_activationType);
}

template <typename Rep, typename Period>
auto TickTimer::reset(const std::chrono::duration<Rep, Period> &timeout, const ActivationType activationType)
{
    return reset(timeout, m_type, activationType);
}

template <typename Rep, typename Period>
auto TickTimer::reset(const std::chrono::duration<Rep, Period> &timeout, const Type type, const ActivationType activationType)
{
    Error error{deactivate()};
    if (error != Error::success)
    {
        return error;
    }

    error = Error{tx_timer_change(this, type == Type::periodicImmediate ? 1UL : ticks(timeout), type == Type::oneShot ? 0UL : ticks(timeout))};
    if (error != Error::success)
    {
        return error;
    }

    m_timeoutTicks = ticks(timeout);
    m_type = type;
    m_activationType = activationType;

    if (activationType == ActivationType::autoActivate)
    {
        return activate();
    }

    return Error::success;
}
} // namespace ThreadX
