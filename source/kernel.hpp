#pragma once

#include "txCommon.hpp"

namespace ThreadX::Native
{
#include "tx_initialize.h"
#include "tx_thread.h"

} // namespace ThreadX::Native

namespace ThreadX::Kernel
{
/// Basic lockable class that prevents task and interrupt context switches while locked.
/// it can either be used as a scoped object or for freely lock/unlucking.
class CriticalSection
{
  public:
    CriticalSection();
    ~CriticalSection();
    /// Locks the CPU, preventing thread and interrupt switches.
    static void lock();
    /// Unlocks the CPU, allowing other interrupts and threads to preempt the current execution context.
    static void unlock();

  private:
    static bool locked;
    static Native::TX_INTERRUPT_SAVE_AREA
};

void start();

///
/// \return true if it is called in a thread
bool inThread();

/// Determines if the current execution context is inside an interrupt service routine.
/// \return true if the current execution context is ISR, false otherwise

bool inIsr();
}; // namespace ThreadX::Kernel

namespace ThreadX
{
[[gnu::weak]] void application(void *firstUnusedMemory);
}
