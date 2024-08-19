#include "norFlash.hpp"
#include <utility>

namespace LevelX
{
NorFlashBase::NorFlashBase(const Driver &driver) : ThreadX::Native::LX_NOR_FLASH{}, m_driver{driver}
{
}

NorFlashBase::~NorFlashBase()
{
    [[maybe_unused]] Error error{close()};
    assert(error == Error::success);
}

void NorFlashBase::init(std::span<ThreadX::Ulong> extendedCacheMemory, const ThreadX::Ulong storageSize,
                        const ThreadX::Ulong blockSize, const ThreadX::Ulong baseAddress)
{
    assert(storageSize % blockSize == 0);
    assert(blockSize % (m_sectorSizeInWord * ThreadX::wordSize) == 0);

    if (not m_initialised.test_and_set())
    {
        ThreadX::Native::lx_nor_flash_initialize();
    }

    if (auto cacheSize{extendedCacheMemory.size()}; cacheSize > 0)
    {
        [[maybe_unused]] Error error{lx_nor_flash_extended_cache_enable(this, extendedCacheMemory.data(), cacheSize)};
        assert(error == Error::success);
    }

    lx_nor_flash_driver_read = DriverCallbacks::read;
    lx_nor_flash_driver_write = DriverCallbacks::write;
    lx_nor_flash_driver_block_erase = DriverCallbacks::eraseBlock;
    lx_nor_flash_driver_block_erased_verify = DriverCallbacks::verifyErasedBlock;
    lx_nor_flash_driver_system_error = DriverCallbacks::systemError;

    lx_nor_flash_base_address = reinterpret_cast<ThreadX::Ulong *>(baseAddress);
    lx_nor_flash_total_blocks = storageSize / blockSize;
    lx_nor_flash_words_per_block = blockSize / ThreadX::wordSize;
    lx_nor_flash_sector_buffer = m_sectorBuffer.data();
}

ThreadX::Ulong NorFlashBase::formatSize() const
{
    return (lx_nor_flash_total_blocks - 1) * (lx_nor_flash_words_per_block * ThreadX::wordSize);
}

Error NorFlashBase::open()
{
    return Error{lx_nor_flash_open(this, const_cast<char *>("nor flash"), DriverCallbacks::initialise)};
}

Error NorFlashBase::close()
{
    return Error{lx_nor_flash_close(this)};
}

Error NorFlashBase::defragment()
{
    return Error{lx_nor_flash_defragment(this)};
}

Error NorFlashBase::defragment(const ThreadX::Uint numberOfBlocks)
{
    return Error{lx_nor_flash_partial_defragment(this, numberOfBlocks)};
}

Error NorFlashBase::readSector(
    const ThreadX::Ulong sectorNumber, std::span<ThreadX::Ulong, m_sectorSizeInWord> sectorData)
{
    return Error{lx_nor_flash_sector_read(this, sectorNumber, sectorData.data())};
}

Error NorFlashBase::releaseSector(const ThreadX::Ulong sectorNumber)
{
    return Error{lx_nor_flash_sector_release(this, sectorNumber)};
}

Error NorFlashBase::writeSector(
    const ThreadX::Ulong sectorNumber, std::span<ThreadX::Ulong, m_sectorSizeInWord> sectorData)
{
    return Error{lx_nor_flash_sector_write(this, sectorNumber, sectorData.data())};
}

ThreadX::Uint NorFlashBase::DriverCallbacks::initialise(ThreadX::Native::LX_NOR_FLASH *norFlashPtr)
{
    if (auto &norFlash{static_cast<NorFlashBase &>(*norFlashPtr)}; norFlash.m_driver.initialiseCallback)
    {
        return norFlash.m_driver.initialiseCallback(norFlashPtr);
    }

    return LX_SUCCESS;
}

ThreadX::Uint NorFlashBase::DriverCallbacks::read(
    ThreadX::Native::LX_NOR_FLASH *norFlashPtr, ThreadX::Ulong *flashAddress, ThreadX::Ulong *destination,
    ThreadX::Ulong words)
{
    auto &norFlash{static_cast<NorFlashBase &>(*norFlashPtr)};
    return norFlash.m_driver.readCallback(norFlashPtr, flashAddress, destination, words);
}

ThreadX::Uint NorFlashBase::DriverCallbacks::write(
    ThreadX::Native::LX_NOR_FLASH *norFlashPtr, ThreadX::Ulong *flashAddress, ThreadX::Ulong *source,
    ThreadX::Ulong words)
{
    auto &norFlash{static_cast<NorFlashBase &>(*norFlashPtr)};
    return norFlash.m_driver.writeCallback(norFlashPtr, flashAddress, source, words);
}

ThreadX::Uint NorFlashBase::DriverCallbacks::eraseBlock(
    ThreadX::Native::LX_NOR_FLASH *norFlashPtr, ThreadX::Ulong block, ThreadX::Ulong eraseCount)
{
    auto &norFlash{static_cast<NorFlashBase &>(*norFlashPtr)};
    return norFlash.m_driver.eraseBlockCallback(norFlashPtr, block, eraseCount);
}

ThreadX::Uint NorFlashBase::DriverCallbacks::verifyErasedBlock(
    ThreadX::Native::LX_NOR_FLASH *norFlashPtr, ThreadX::Ulong block)
{
    auto &norFlash{static_cast<NorFlashBase &>(*norFlashPtr)};
    return norFlash.m_driver.verifyErasedBlockCallback(norFlashPtr, block);
}

ThreadX::Uint NorFlashBase::DriverCallbacks::systemError(
    ThreadX::Native::LX_NOR_FLASH *norFlashPtr, ThreadX::Uint errorCode)
{
    if (auto &norFlash{static_cast<NorFlashBase &>(*norFlashPtr)}; norFlash.m_driver.systemErrorCallback)
    {
        norFlash.m_driver.systemErrorCallback(norFlashPtr, errorCode);
    }

    return LX_SUCCESS;
}
} // namespace LevelX