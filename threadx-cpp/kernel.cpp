#include "kernel.hpp"

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
    lock();
}

CriticalSection::~CriticalSection()
{
    unlock();
}

auto CriticalSection::lock() -> void
{
    if (not m_locked.test_and_set())
    {
        using namespace Native;
        TX_DISABLE
    }
}

auto CriticalSection::unlock() -> void
{
    if (m_locked.test())
    {
        Native::TX_RESTORE;
        m_locked.clear();
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
