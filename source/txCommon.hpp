#pragma once

#include <cstdint>
#include <cstdlib>
#include <cstring>

namespace ThreadX::Native
{
#include "fx_api.h"
#include "tx_api.h"
#ifdef FX_ENABLE_FAULT_TOLERANT
#include "fx_fault_tolerant.h"
#endif
} // namespace ThreadX::Native

namespace ThreadX
{
using Ulong = Native::ULONG;     //uint32_t
using Ulong64 = Native::ULONG64; //uint64_t
using Uint = Native::UINT;       //size_t

inline constexpr auto sizeOfUlong{sizeof(Ulong)};
static_assert(sizeOfUlong >= sizeof(uintptr_t));

enum class Error : Uint
{
    success,
    deleted,
    poolError,
    ptrError,
    waitError,
    sizeError,
    groupError,
    noEvents,
    optionError,
    queueError,
    queueEmpty,
    queueFull,
    semaphoreError,
    noInstance,
    threadError,
    priorityError,
    noMemory,
    startError = noMemory,
    deleteError,
    resumeError,
    callerError,
    suspendError,
    timerError,
    tickError,
    activateError,
    threshError,
    suspendLifted,
    waitAborted,
    waitAbortError,
    mutexError,
    notAvailable,
    notOwned,
    inheretError,
    notDone,
    ceilingExceeded,
    invalidCeiling,
    featureNotEnabled = 0xFF
};
} // namespace ThreadX

enum class Error : size_t
{
    none = 0,
    nullPtrAccess,
    lengthError,
    invalidArgument,
    alreadyInitialised,
    allocationError,
    unexpectedValue,
    unknownError
};
