#pragma once

#include <cstdint>
#include <cstdlib>
#include <cstring>

namespace ThreadX::Native
{
#include "tx_api.h"
} // namespace ThreadX::Native

namespace ThreadX
{
using Ulong = Native::ULONG;     //uint32_t
using Ulong64 = Native::ULONG64; //uint64_t
using Uint = Native::UINT;       //size_t
using Uchar = Native::UCHAR;

constexpr auto wordSize{sizeof(Ulong)};
static_assert(wordSize >= sizeof(uintptr_t));

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
