#pragma once

#include "txCommon.hpp"

namespace ThreadX::Native
{
#include "lx_api.h"
} // namespace ThreadX::Native

namespace LevelX
{
enum class Error : ThreadX::Uint
{
    success = LX_SUCCESS,
    error = LX_ERROR,
    noSectors = LX_NO_SECTORS,
    sectorNotFound = LX_SECTOR_NOT_FOUND,
    noPages = LX_NO_PAGES,
    invalidWrite = LX_INVALID_WRITE,
    nandErrorCorrected = LX_NAND_ERROR_CORRECTED,
    nandErrorNotCorrected = LX_NAND_ERROR_NOT_CORRECTED,
    noMemory = LX_NO_MEMORY,
    disabled = LX_DISABLED,
    badBlock = LX_BAD_BLOCK,
    noBlocks = LX_NO_BLOCKS,
    notSupported = LX_NOT_SUPPORTED,
    systemInvalidFormat = LX_SYSTEM_INVALID_FORMAT,
    systemInvalidBlock = LX_SYSTEM_INVALID_BLOCK,
    systemAllocationFailed = LX_SYSTEM_ALLOCATION_FAILED,
    systemMutexCreateFailed = LX_SYSTEM_MUTEX_CREATE_FAILED,
    systemInvalidSectorMap = LX_SYSTEM_INVALID_SECTOR_MAP
};
} // namespace LevelX
