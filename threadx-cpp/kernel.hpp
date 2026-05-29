#pragma once

#include "txCommon.hpp"

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
    CriticalSection();
    ~CriticalSection();

    CriticalSection(const CriticalSection &) = delete;
    CriticalSection &operator=(const CriticalSection &) = delete;
    CriticalSection(CriticalSection &&) = delete;
    CriticalSection &operator=(CriticalSection &&) = delete;

  private:
    static inline Uint m_nestingCount;
    static inline Uint m_savedInterruptState;
};

auto start() -> void;

///
/// \return true if it is called in a thread
[[nodiscard]] auto inThread() -> bool;

/// Determines if the current execution context is inside an interrupt service routine.
/// \return true if the current execution context is ISR, false otherwise
[[nodiscard]] auto inIsr() -> bool;

[[nodiscard]] auto state() -> State;
} // namespace ThreadX::Kernel
