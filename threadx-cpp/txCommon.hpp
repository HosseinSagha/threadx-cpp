#pragma once

#include <cstdint>
#include <cstdlib>
#include <cstring>

// This file contains common type definitions and error enumerations for ThreadX.

namespace ThreadX::Native
{
#include "tx_api.h"
} // namespace ThreadX::Native

namespace ThreadX
{
using Uchar = Native::UCHAR;
using Uint = Native::UINT;
using Ulong = Native::ULONG;
using Ulong64 = Native::ULONG64;

inline constexpr auto wordSize{sizeof(Ulong)};

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
