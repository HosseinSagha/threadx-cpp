#pragma once

#include "test.hpp"
#include "threadx-cpp/fxCommon.hpp"
#include "threadx-cpp/lxCommon.hpp"

class NorMedia;

LevelX::Error norFlashSimulatorEraseAll();

#if 0
LevelX::Error nandFlashSimulatorRead(
    NandFlash *nandFlash, ThreadX::Ulong block, ThreadX::Ulong page, ThreadX::Ulong *destination, ThreadX::Ulong words);
LevelX::Error nandFlashSimulatorPagesRead(
    NandFlash *nandFlash, ThreadX::Ulong block, ThreadX::Ulong page, ThreadX::Uchar *main_buffer,
    ThreadX::Uchar *spare_buffer, ThreadX::Ulong pages);
LevelX::Error nandFlashSimulatorWrite(
    NandFlash *nandFlash, ThreadX::Ulong block, ThreadX::Ulong page, ThreadX::Ulong *source, ThreadX::Ulong words);
LevelX::Error nandFlashSimulatorPagesWrite(
    NandFlash *nandFlash, ThreadX::Ulong block, ThreadX::Ulong page, ThreadX::Uchar *main_buffer,
    ThreadX::Uchar *spare_buffer, ThreadX::Ulong pages);
LevelX::Error nandFlashSimulatorPagesCopy(
    NandFlash *nandFlash, ThreadX::Ulong source_block, ThreadX::Ulong source_page, ThreadX::Ulong destination_block,
    ThreadX::Ulong destination_page, ThreadX::Ulong pages, ThreadX::Uchar *data_buffer);
LevelX::Error nandFlashSimulatorBlockErase(ThreadX::Ulong block, ThreadX::Ulong erase_count);
LevelX::Error nandFlashSimulatorEraseAll();
LevelX::Error nandFlashSimulatorBlockErasedVerify(ThreadX::Ulong block);
LevelX::Error nandFlashSimulatorPageErasedVerify(ThreadX::Ulong block, ThreadX::Ulong page);
LevelX::Error nandFlashSimulatorBlockStatusGet(ThreadX::Ulong block, ThreadX::Uchar *bad_block_byte);
LevelX::Error nandFlashSimulatorBlockStatusSet(ThreadX::Ulong block, ThreadX::Uchar bad_block_byte);
LevelX::Error nandFlashSimulatorExtraBytesGet(
    ThreadX::Ulong block, ThreadX::Ulong page, ThreadX::Uchar *destination, ThreadX::Uint size);
LevelX::Error nandFlashSimulatorExtraBytesSet(
    ThreadX::Ulong block, ThreadX::Ulong page, ThreadX::Uchar *source, ThreadX::Uint size);
#endif
