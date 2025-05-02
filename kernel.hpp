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

namespace ThreadX::Kernel
{
enum class State
{
    uninitialised,
    running
};

/// Basic lockable class that prevents task and interrupt context switches while locked.
/// it can either be used as a scoped object or for freely lock/unlucking.
class CriticalSection final
{
  public:
    explicit CriticalSection();
    ~CriticalSection();
    /// Locks the CPU, preventing thread and interrupt switches.
    static void lock();
    /// Unlocks the CPU, allowing other interrupts and threads to preempt the current execution context.
    static void unlock();

  private:
    static inline std::atomic_flag m_locked = ATOMIC_FLAG_INIT;
    static inline Native::TX_INTERRUPT_SAVE_AREA
};

void start();

///
/// \return true if it is called in a thread
bool inThread();

/// Determines if the current execution context is inside an interrupt service routine.
/// \return true if the current execution context is ISR, false otherwise

bool inIsr();

State state();
}; // namespace ThreadX::Kernel

namespace ThreadX
{
[[gnu::weak]] void application();
}
