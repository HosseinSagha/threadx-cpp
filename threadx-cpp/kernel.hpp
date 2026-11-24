#pragma once

#include "txCommon.hpp"

#include <atomic>

namespace ThreadX::Native
{
extern "C" {
#include "tx_initialize.h"
#include "tx_thread.h"
}
} // namespace ThreadX::Native

namespace ThreadX
{
auto application() -> void;
}

namespace ThreadX::Kernel
{
enum class State
{
    uninitialised,
    running
};

/// RAII-based critical section for interrupt-safe resource access.
/// Disables interrupts while locked - use for protecting resources accessed by both
/// application threads and interrupt handlers.
///
/// Note: Interrupts are globally disabled - avoid holding for long periods.
class CriticalSection final
{
  public:
    explicit CriticalSection();
    ~CriticalSection() noexcept;

    CriticalSection(const CriticalSection &) = delete;
    CriticalSection &operator=(const CriticalSection &) = delete;
    CriticalSection(CriticalSection &&) = delete;
    CriticalSection &operator=(CriticalSection &&) = delete;

  private:
    static inline std::atomic<Uint> m_nestingCount{};
    static inline Uint m_savedInterruptState{};
};

auto start() -> void;

///
/// \return true if it is called in a thread
[[nodiscard]] auto inThread() -> bool;

/// Determines if the current execution context is inside an interrupt service routine.
/// \return true if the current execution context is ISR, false otherwise
[[nodiscard]] auto inIsr() -> bool;

[[nodiscard]] auto state() -> State;

#if defined(__CORTEX_M)

#if defined(__NVIC_PRIO_BITS)
static constexpr auto basepriForMaxPreemptPrio(Uint maxPreemptPrio) -> Uint
{
    return (maxPreemptPrio + 1U) << (8U - __NVIC_PRIO_BITS);
}
#endif

/// RAII-based priority-masked critical section for Cortex-M.
/// Raises BASEPRI to mask interrupts at or below the given priority level while
/// allowing higher-priority ISRs to run. Safe for nesting within a single thread:
/// each instance saves and restores its own BASEPRI value in LIFO order.
///
/// \param basepriLevel Value written to BASEPRI (0 disables masking).
///                     Interrupts with priority >= level are masked.
///                     Use basepriForMaxPreemptPrio() to pre-shift the level
///                     when __NVIC_PRIO_BITS is defined.
///
/// \warning BASEPRI is a per-core register shared by all threads on the core,
///          not per-thread. Do not construct/destruct instances across thread
///          boundaries or hold one across a yield/blocking call — interleaved
///          lifetimes between threads corrupt the saved/restore state and leave
///          the core at the wrong priority. For thread-to-thread mutual
///          exclusion use a ThreadX mutex instead.
class MaskedCriticalSection final
{
  public:
    explicit MaskedCriticalSection(Uint basepriLevel);
    ~MaskedCriticalSection() noexcept;

    MaskedCriticalSection(const MaskedCriticalSection &) = delete;
    MaskedCriticalSection &operator=(const MaskedCriticalSection &) = delete;
    MaskedCriticalSection(MaskedCriticalSection &&) = delete;
    MaskedCriticalSection &operator=(MaskedCriticalSection &&) = delete;

  private:
    static auto readBasepri() -> Uint;
    static auto writeBasepri(Uint level) -> void;

    Uint m_savedBasepri{};
};

#endif
} // namespace ThreadX::Kernel
