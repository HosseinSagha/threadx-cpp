#pragma once

#include "test.hpp"
#include "threadx-cpp/fxCommon.hpp"
#include "threadx-cpp/lxCommon.hpp"

class NorMedia;

extern std::byte ramMem[10 * 512];

void ramMediaDriver(FileX::Media<>::InternalDriver &ramMedia);
void norFlashSimulatorMediaDriver(FileX::Media<NorFlash::sectorSize()>::InternalDriver &norMedia, NorFlash &norFlash);
#if 0
void nandFlashSimulatorMediaDriver(NandMedia &norMedia);
#endif
