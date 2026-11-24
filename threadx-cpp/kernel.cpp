#include "kernel.hpp"
#include <cassert>

namespace ThreadX
{
auto Native::tx_application_define([[maybe_unused]] void *firstUnusedMemory) -> void
{
    application();
}
} // namespace ThreadX

namespace ThreadX::Kernel
{
CriticalSection::CriticalSection()
{
    using namespace Native;
    TX_INTERRUPT_SAVE_AREA;
    TX_DISABLE;

    if (m_nestingCount.fetch_add(1) == 0)
    {
        m_savedInterruptState = interrupt_save;
    }
}

CriticalSection::~CriticalSection() noexcept
{
    const auto previous{m_nestingCount.fetch_sub(1)};
    assert(previous >= 1);

    if (previous == 1)
    {
        using namespace Native;
        TX_INTERRUPT_SAVE_AREA;
        interrupt_save = m_savedInterruptState;
        TX_RESTORE;
    }
}

auto start() -> void
{
    Native::tx_kernel_enter();
}

auto inThread() -> bool
{
    return Native::tx_thread_identify() ? true : false;
}

auto inIsr() -> bool
{
    using namespace Native;
    const Ulong systemState{TX_THREAD_GET_SYSTEM_STATE()};
    return systemState > TX_INITIALIZE_IS_FINISHED and systemState < TX_INITIALIZE_IN_PROGRESS;
}

auto state() -> State
{
    using namespace Native;
    return (TX_THREAD_GET_SYSTEM_STATE() < TX_INITIALIZE_IN_PROGRESS) ? State::running : State::uninitialised;
}

#if defined(__CORTEX_M)

auto MaskedCriticalSection::readBasepri() -> Uint
{
#if defined(__GNUC__) || defined(__clang__)
    Uint result{};
    __asm volatile ("mrs %0, basepri" : "=r" (result));
    return result;
#else
#error "MaskedCriticalSection requires GCC or Clang inline assembly for BASEPRI access"
#endif
}

auto MaskedCriticalSection::writeBasepri(Uint level) -> void
{
#if defined(__GNUC__) || defined(__clang__)
    __asm volatile ("msr basepri, %0" : : "r" (level) : "memory");
#else
#error "MaskedCriticalSection requires GCC or Clang inline assembly for BASEPRI access"
#endif
}

MaskedCriticalSection::MaskedCriticalSection(Uint basepriLevel)
    : m_savedBasepri(readBasepri())
{
    writeBasepri(basepriLevel);
}

MaskedCriticalSection::~MaskedCriticalSection() noexcept
{
    writeBasepri(m_savedBasepri);
}

#endif
} // namespace ThreadX::Kernel
