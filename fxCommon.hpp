#pragma once

// This file contains common definitions and enumerations for FileX.

#include "txCommon.hpp"

namespace ThreadX::Native
{
#include "fx_api.h"
#ifdef FX_ENABLE_FAULT_TOLERANT
#include "fx_fault_tolerant.h"
#endif
} // namespace ThreadX::Native

namespace FileX
{
enum class MediaSectorSize : ThreadX::Uint
{
    halfAKilobyte = 512,
    oneKiloByte = halfAKilobyte * 2,
    twoKiloBytes = oneKiloByte * 2,
    fourKilobytes = twoKiloBytes * 2
};

enum class Error : ThreadX::Uint
{
    success = FX_SUCCESS,
    bootError = FX_BOOT_ERROR,
    mediaInvalid = FX_MEDIA_INVALID,
    fatReadError = FX_FAT_READ_ERROR,
    notFound = FX_NOT_FOUND,
    notAFile = FX_NOT_A_FILE,
    accessError = FX_ACCESS_ERROR,
    notOpen = FX_NOT_OPEN,
    fileCorrupt = FX_FILE_CORRUPT,
    endOfFile = FX_END_OF_FILE,
    noMoreSpace = FX_NO_MORE_SPACE,
    alreadyCreated = FX_ALREADY_CREATED,
    invalidName = FX_INVALID_NAME,
    invalidPath = FX_INVALID_PATH,
    notDirectory = FX_NOT_DIRECTORY,
    noMoreEntries = FX_NO_MORE_ENTRIES,
    dirNotEmpty = FX_DIR_NOT_EMPTY,
    mediaNotOpen = FX_MEDIA_NOT_OPEN,
    invalidYear = FX_INVALID_YEAR,
    invalidMonth = FX_INVALID_MONTH,
    invalidDay = FX_INVALID_DAY,
    invalidHour = FX_INVALID_HOUR,
    invalidMinute = FX_INVALID_MINUTE,
    invalidSecond = FX_INVALID_SECOND,
    ptrError = FX_PTR_ERROR,
    invalidAttr = FX_INVALID_ATTR,
    callerError = FX_CALLER_ERROR,
    bufferError = FX_BUFFER_ERROR,
    notImplemented = FX_NOT_IMPLEMENTED,
    writeProtect = FX_WRITE_PROTECT,
    invalidOption = FX_INVALID_OPTION,
    sectorInvalid = FX_SECTOR_INVALID,
    ioError = FX_IO_ERROR,
    notEnoughMemory = FX_NOT_ENOUGH_MEMORY,
    errorFixed = FX_ERROR_FIXED,
    errorNotFixed = FX_ERROR_NOT_FIXED,
    notAvailable = FX_NOT_AVAILABLE,
    invalidChecksum = FX_INVALID_CHECKSUM,
    readContinue = FX_READ_CONTINUE,
    invalidState = FX_INVALID_STATE
};

inline constexpr auto defaultSectorSize{MediaSectorSize::halfAKilobyte};
} // namespace FileX
