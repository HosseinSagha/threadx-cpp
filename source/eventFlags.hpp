#include "tickTimer.hpp"
#include "txCommon.hpp"
#include <bitset>
#include <climits>
#include <functional>
#include <tuple>

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
    using BitMask = std::bitset<EventFlags::eventFlagBit>;
    using ReturnTuple = std::tuple<Error, BitMask>;

    static constexpr auto allBits{BitMask{std::numeric_limits<Ulong>::max()}};

    ///
    /// \param setNotifyCallback set notify callback. \sa NotifyCallback
    EventFlags(const NotifyCallback &setNotifyCallback = {});

    ~EventFlags();

    /// \param bitMask flag bitmask to set
    Error set(const BitMask &bitMask);

    /// \param bitMask flag bitmask to clear
    Error clear(const BitMask &bitMask = allBits);

    // must be used for calls from initialization, timers, and ISRs
    ReturnTuple get(const BitMask &bitMask = allBits, const EventOption eventOption = EventOption::clear);

    ReturnTuple waitAll(const BitMask &bitMask, const EventOption eventOption = EventOption::clear);

    ReturnTuple waitAllFor(const BitMask &bitMask, const TickTimer::Duration &waitDuration,
                           const EventOption eventOption = EventOption::clear);

    ReturnTuple waitAllUntil(const BitMask &bitMask, const TickTimer::TimePoint &timePoint,
                             const EventOption eventOption = EventOption::clear);

    ReturnTuple waitAny(const BitMask &bitMask, const EventOption eventOption = EventOption::clear);

    ReturnTuple waitAnyFor(const BitMask &bitMask, const TickTimer::Duration &waitDuration,
                           const EventOption eventOption = EventOption::clear);

    ReturnTuple waitAnyUntil(const BitMask &bitMask, const TickTimer::TimePoint &timePoint,
                             const EventOption eventOption = EventOption::clear);

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
    ReturnTuple waitFor(const BitMask &bitMask, const TickTimer::Duration &waitDuration, const Option option);

    static void setNotifyCallback(auto notifyGroupPtr);

    const NotifyCallback m_setNotifyCallback;
};
} // namespace ThreadX
