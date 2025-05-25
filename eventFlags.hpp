#pragma once

#include "tickTimer.hpp"
#include "txCommon.hpp"
#include <bitset>
#include <climits>
#include <expected>
#include <functional>
#include <string_view>

namespace ThreadX
{
/// Set and wait on event flags
class EventFlags final : Native::TX_EVENT_FLAGS_GROUP
{
  public:
    enum class Option
    {
        keep,
        clear
    };

    static constexpr size_t eventFlagBits{wordSize * CHAR_BIT};
    /// external callback type
    using NotifyCallback = std::function<void(EventFlags &)>;
    using Bitmask = std::bitset<EventFlags::eventFlagBits>;
    using ExpectedBitmask = std::expected<Bitmask, Error>;

    static constexpr auto allBits{Bitmask{std::numeric_limits<Ulong>::max()}};

    ///
    /// \param setNotifyCallback set notify callback. \sa NotifyCallback
    explicit EventFlags(const std::string_view name, const NotifyCallback &setNotifyCallback = {});

    ~EventFlags();

    /// \param bitMask flag bitmask to set
    auto set(const Bitmask &bitMask) -> Error;

    /// \param bitMask flag bitmask to clear
    auto clear(const Bitmask &bitMask = allBits) -> Error;

    /// must be used for calls from initialization, timers, and ISRs
    ///\param option
    ///\return ExpectedBitmask
    /// no error returned in case of no events
    [[nodiscard]] auto get(const Option option = Option::clear) -> ExpectedBitmask;

    /// must be used for calls from initialization, timers, and ISRs
    ///\param bitMask
    ///\param option
    ///\return ExpectedBitmask
    /// no error returned in case of no events
    [[nodiscard]] auto get(const Bitmask &bitMask, const Option option = Option::clear) -> ExpectedBitmask;

    ///
    ///\param bitMask
    ///\param option
    ///\return ExpectedBitmask, unexpected if error:
    /// waitAborted: if Suspension was aborted by another thread, timer, or ISR.
    auto waitAll(const Bitmask &bitMask, const Option option = Option::clear) -> ExpectedBitmask;

    ///
    ///\tparam Clock
    ///\tparam Duration
    ///\param bitMask
    ///\param time
    ///\param option
    ///\return ExpectedBitmask
    /// noEvents: if it was unable to get the specified events within the specified timeout
    /// waitAborted: if Suspension was aborted by another thread, timer, or ISR.
    template <class Clock, typename Duration>
    auto waitAllUntil(const Bitmask &bitMask, const std::chrono::time_point<Clock, Duration> &time, const Option option = Option::clear)
        -> ExpectedBitmask;

    ///
    ///\tparam Rep
    ///\tparam Period
    ///\param bitMask
    ///\param duration
    ///\param option
    ///\return ExpectedBitmask
    /// noEvents: if it was unable to get the specified events within the specified timeout
    /// waitAborted: if Suspension was aborted by another thread, timer, or ISR.
    template <typename Rep, typename Period>
    auto waitAllFor(const Bitmask &bitMask, const std::chrono::duration<Rep, Period> &duration, const Option option = Option::clear)
        -> ExpectedBitmask;

    ///
    ///\param bitMask
    ///\param option
    ///\return ExpectedBitmask
    /// waitAborted: if Suspension was aborted by another thread, timer, or ISR.
    auto waitAny(const Bitmask &bitMask, const Option option = Option::clear) -> ExpectedBitmask;

    ///
    ///\tparam Clock
    ///\tparam Duration
    ///\param bitMask
    ///\param time
    ///\param option
    ///\return ExpectedBitmask
    /// noEvents: if it was unable to get the specified events within the specified timeout
    /// waitAborted: if Suspension was aborted by another thread, timer, or ISR.
    template <class Clock, typename Duration>
    auto waitAnyUntil(const Bitmask &bitMask, const std::chrono::time_point<Clock, Duration> &time, const Option option = Option::clear)
        -> ExpectedBitmask;

    ///
    ///\tparam Rep
    ///\tparam Period
    ///\param bitMask
    ///\param duration
    ///\param option
    ///\return ExpectedBitmask
    /// noEvents: if it was unable to get the specified events within the specified timeout
    /// waitAborted: if Suspension was aborted by another thread, timer, or ISR.
    template <typename Rep, typename Period>
    auto waitAnyFor(const Bitmask &bitMask, const std::chrono::duration<Rep, Period> &duration, const Option option = Option::clear)
        -> ExpectedBitmask;

    [[nodiscard]] auto name() const -> std::string_view;

  private:
    enum class FlagOption : Uint
    {
        any, ///< any resume, if any flag in bitmask is set
        orInto = any,
        anyClear, ///< anyClear resume, if any flag in bitmask is set and then clear.
        all,      ///< all resume, if all flags in bitmask are set
        andInto = all,
        allClear ///< allClear resume, if all flags in bitmask are set and then clear.
    };

    static auto setNotifyCallback(Native::TX_EVENT_FLAGS_GROUP *notifyGroupPtr) -> void;

    /// \param bitMask flag bitmask to get
    /// \param duration Wait duration
    /// \param option \sa Option
    /// \return actual flags set
    auto waitFor(const Bitmask &bitMask, const auto &duration, const FlagOption flagOption) -> ExpectedBitmask;

    const NotifyCallback m_setNotifyCallback;
};

template <class Clock, typename Duration>
auto EventFlags::waitAllUntil(const Bitmask &bitMask, const std::chrono::time_point<Clock, Duration> &time, const Option option)
    -> ExpectedBitmask
{
    return waitAllFor(bitMask, time - Clock::now(), option);
}

template <typename Rep, typename Period>
auto EventFlags::waitAllFor(const Bitmask &bitMask, const std::chrono::duration<Rep, Period> &duration, const Option option)
    -> ExpectedBitmask
{
    auto flagOption{FlagOption::allClear};
    if (option == Option::keep)
    {
        flagOption = FlagOption::all;
    }

    return waitFor(bitMask, duration, flagOption);
}

template <class Clock, typename Duration>
auto EventFlags::waitAnyUntil(const Bitmask &bitMask, const std::chrono::time_point<Clock, Duration> &time, const Option option)
    -> ExpectedBitmask
{
    return waitAnyFor(bitMask, time - Clock::now(), option);
}

template <typename Rep, typename Period>
auto EventFlags::waitAnyFor(const Bitmask &bitMask, const std::chrono::duration<Rep, Period> &duration, const Option option)
    -> ExpectedBitmask
{
    auto flagOption{FlagOption::anyClear};
    if (option == Option::keep)
    {
        flagOption = FlagOption::any;
    }

    return waitFor(bitMask, duration, flagOption);
}

auto EventFlags::waitFor(const Bitmask &bitMask, const auto &duration, const FlagOption flagOption) -> ExpectedBitmask
{
    Ulong actualFlags{};
    if (Error error{tx_event_flags_get(
            this, bitMask.to_ulong(), std::to_underlying(flagOption), std::addressof(actualFlags), TickTimer::ticks(duration))};
        error != Error::success)
    {
        return std::unexpected(error);
    }

    return Bitmask{actualFlags};
}
} // namespace ThreadX
