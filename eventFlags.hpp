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
        dontClear,
        clear
    };

    static constexpr size_t eventFlagBit{wordSize * CHAR_BIT};
    /// external callback type
    using NotifyCallback = std::function<void(EventFlags &)>;
    using Bitmask = std::bitset<EventFlags::eventFlagBit>;
    using ExpectedBitmask = std::expected<Bitmask, Error>;

    static constexpr auto allBits{Bitmask{std::numeric_limits<Ulong>::max()}};

    ///
    /// \param setNotifyCallback set notify callback. \sa NotifyCallback
    explicit EventFlags(const std::string_view name, const NotifyCallback &setNotifyCallback = {});

    ~EventFlags();

    /// \param bitMask flag bitmask to set
    Error set(const Bitmask &bitMask);

    /// \param bitMask flag bitmask to clear
    Error clear(const Bitmask &bitMask = allBits);

    // must be used for calls from initialization, timers, and ISRs
    ExpectedBitmask get(const Bitmask &bitMask = allBits, const Option option = Option::clear);

    ExpectedBitmask waitAll(const Bitmask &bitMask, const Option option = Option::clear);

    template <class Clock, typename Duration>
    auto waitAllUntil(const Bitmask &bitMask, const std::chrono::time_point<Clock, Duration> &time, const Option option = Option::clear);

    template <typename Rep, typename Period>
    auto waitAllFor(const Bitmask &bitMask, const std::chrono::duration<Rep, Period> &duration, const Option option = Option::clear);

    ExpectedBitmask waitAny(const Bitmask &bitMask, const Option option = Option::clear);

    template <class Clock, typename Duration>
    auto waitAnyUntil(const Bitmask &bitMask, const std::chrono::time_point<Clock, Duration> &time, const Option option = Option::clear);

    template <typename Rep, typename Period>
    auto waitAnyFor(const Bitmask &bitMask, const std::chrono::duration<Rep, Period> &duration, const Option option = Option::clear);

    std::string_view name() const;

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

    /// \param bitMask flag bitmask to get
    /// \param duration Wait duration
    /// \param option \sa Option
    /// \return actual flags set
    ExpectedBitmask waitFor(const Bitmask &bitMask, const auto &duration, const FlagOption flagOption);

    static void setNotifyCallback(Native::TX_EVENT_FLAGS_GROUP *notifyGroupPtr);

    const NotifyCallback m_setNotifyCallback;
};

template <class Clock, typename Duration>
auto EventFlags::waitAllUntil(const Bitmask &bitMask, const std::chrono::time_point<Clock, Duration> &time, const Option option)
{
    return waitAllFor(bitMask, time - Clock::now(), option);
}

template <typename Rep, typename Period>
auto EventFlags::waitAllFor(const Bitmask &bitMask, const std::chrono::duration<Rep, Period> &duration, const Option option)
{
    auto flagOption{FlagOption::allClear};
    if (option == Option::dontClear)
    {
        flagOption = FlagOption::all;
    }

    return waitFor(bitMask, duration, flagOption);
}

template <class Clock, typename Duration>
auto EventFlags::waitAnyUntil(const Bitmask &bitMask, const std::chrono::time_point<Clock, Duration> &time, const Option option)
{
    return waitAnyFor(bitMask, time - Clock::now(), option);
}

template <typename Rep, typename Period>
auto EventFlags::waitAnyFor(const Bitmask &bitMask, const std::chrono::duration<Rep, Period> &duration, const Option option)
{
    auto flagOption{FlagOption::anyClear};
    if (option == Option::dontClear)
    {
        flagOption = FlagOption::any;
    }

    return waitFor(bitMask, duration, flagOption);
}

EventFlags::ExpectedBitmask EventFlags::waitFor(const Bitmask &bitMask, const auto &duration, const FlagOption flagOption)
{
    Ulong actualFlags{};
    if (Error error{tx_event_flags_get(this, bitMask.to_ulong(), std::to_underlying(flagOption), std::addressof(actualFlags), TickTimer::ticks(duration))};
        error != Error::success)
    {
        return std::unexpected(error);
    }

    return Bitmask{actualFlags};
}
} // namespace ThreadX
