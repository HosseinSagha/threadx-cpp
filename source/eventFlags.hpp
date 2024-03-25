#include "tickTimer.hpp"
#include "txCommon.hpp"
#include <bitset>
#include <climits>
#include <functional>

namespace ThreadX
{
enum class EventOption
{
    dontClear,
    clear
};

/// Set and wait on event flags
class EventFlags : Native::TX_EVENT_FLAGS_GROUP
{
  public:
    static constexpr size_t eventFlagBit{sizeof(Ulong) * CHAR_BIT};
    /// external callback type
    using NotifyCallback = std::function<void(EventFlags &)>;
    using Bitmask = std::bitset<EventFlags::eventFlagBit>;
    using BitmaskPair = std::pair<Error, Bitmask>;

    static constexpr auto allBits{Bitmask{std::numeric_limits<Ulong>::max()}};

    ///
    /// \param setNotifyCallback set notify callback. \sa NotifyCallback
    EventFlags(const std::string_view name, const NotifyCallback &setNotifyCallback = {});

    ~EventFlags();

    /// \param bitMask flag bitmask to set
    Error set(const Bitmask &bitMask);

    /// \param bitMask flag bitmask to clear
    Error clear(const Bitmask &bitMask = allBits);

    // must be used for calls from initialization, timers, and ISRs
    BitmaskPair get(const Bitmask &bitMask = allBits, const EventOption eventOption = EventOption::clear);

    BitmaskPair waitAll(const Bitmask &bitMask, const EventOption eventOption = EventOption::clear);

    template <class Clock, typename Duration>
    auto waitAllUntil(const Bitmask &bitMask, const std::chrono::time_point<Clock, Duration> &time,
                      const EventOption eventOption = EventOption::clear);

    template <typename Rep, typename Period>
    auto waitAllFor(const Bitmask &bitMask, const std::chrono::duration<Rep, Period> &waitDuration,
                    const EventOption eventOption = EventOption::clear);

    BitmaskPair waitAny(const Bitmask &bitMask, const EventOption eventOption = EventOption::clear);

    template <class Clock, typename Duration>
    auto waitAnyUntil(const Bitmask &bitMask, const std::chrono::time_point<Clock, Duration> &time,
                      const EventOption eventOption = EventOption::clear);

    template <typename Rep, typename Period>
    auto waitAnyFor(const Bitmask &bitMask, const std::chrono::duration<Rep, Period> &waitDuration,
                    const EventOption eventOption = EventOption::clear);

    std::string_view name();

  private:
    enum class Option : Uint
    {
        any, ///< any resume, if any flag in bitmask is set
        orInto = any,
        anyClear, ///< anyClear resume, if any flag in bitmask is set and then clear.
        all,      ///< all resume, if all flags in bitmask are set
        andInto = all,
        allClear ///< allClear resume, if all flags in bitmask are set and then clear.
    };

    /// \param bitMask flag bitmask to get
    /// \param waitDuration Wait duration
    /// \param option \sa Option
    /// \return actual flags set
    BitmaskPair waitFor(const Bitmask &bitMask, const TickTimer::Duration &waitDuration, const Option option);

    static void setNotifyCallback(auto notifyGroupPtr);

    const NotifyCallback m_setNotifyCallback;
};

template <class Clock, typename Duration>
auto EventFlags::waitAllUntil(
    const Bitmask &bitMask, const std::chrono::time_point<Clock, Duration> &time, const EventOption eventOption)
{
    return waitAllFor(bitMask, time - Clock::now(), eventOption);
}

template <typename Rep, typename Period>
auto EventFlags::waitAllFor(
    const Bitmask &bitMask, const std::chrono::duration<Rep, Period> &waitDuration, const EventOption eventOption)
{
    auto option{Option::allClear};
    if (eventOption == EventOption::dontClear)
    {
        option = Option::all;
    }

    return waitFor(bitMask, waitDuration, option);
}

template <class Clock, typename Duration>
auto EventFlags::waitAnyUntil(
    const Bitmask &bitMask, const std::chrono::time_point<Clock, Duration> &time, const EventOption eventOption)
{
    return waitAnyFor(bitMask, time - Clock::now(), eventOption);
}

template <typename Rep, typename Period>
auto EventFlags::waitAnyFor(
    const Bitmask &bitMask, const std::chrono::duration<Rep, Period> &waitDuration, const EventOption eventOption)
{
    auto option{Option::anyClear};
    if (eventOption == EventOption::dontClear)
    {
        option = Option::any;
    }

    return waitFor(bitMask, std::chrono::duration_cast<TickTimer::Duration>(waitDuration), option);
}
} // namespace ThreadX
