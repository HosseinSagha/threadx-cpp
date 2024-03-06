#include "kernel.hpp"

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

void CriticalSection::lock()
{
    if (not locked)
    {
        using namespace Native;
        TX_DISABLE
        locked = true;
    }
}

void CriticalSection::unlock()
{
    if (locked)
    {
        locked = false;
        Native::TX_RESTORE
    }
}

void start()
{
    Native::tx_kernel_enter();
}

bool inThread()
{
    return Native::tx_thread_identify() ? true : false;
}

bool inIsr()
{
    using namespace Native;
    const Ulong systemState{TX_THREAD_GET_SYSTEM_STATE()};
    return systemState != TX_INITIALIZE_IS_FINISHED and systemState < TX_INITIALIZE_IN_PROGRESS;
}

State state()
{
    using namespace Native;
    return (TX_THREAD_GET_SYSTEM_STATE() < TX_INITIALIZE_IN_PROGRESS) ? State::running : State::uninitialised;
}
}; // namespace ThreadX::Kernel

namespace ThreadX
{
void Native::tx_application_define(void *firstUnusedMemory)
{
    application(firstUnusedMemory);
}
} // namespace ThreadX
