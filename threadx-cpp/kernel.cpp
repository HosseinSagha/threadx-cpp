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

    if (m_nestingCount == 0)
    {
        m_savedInterruptState = interrupt_save;
    }

    m_nestingCount = m_nestingCount + 1;
}

CriticalSection::~CriticalSection()
{
    m_nestingCount = m_nestingCount - 1;

    if (m_nestingCount == 0)
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
} // namespace ThreadX::Kernel
