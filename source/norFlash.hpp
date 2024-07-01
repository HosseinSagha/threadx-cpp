#pragma once

#include "lxCommon.hpp"
#include "media.hpp"
#include <array>
#include <atomic>
#include <functional>
#include <span>

namespace LevelX
{
class NorFlashBase : protected ThreadX::Native::LX_NOR_FLASH
{
  public:
    struct Driver
    {
        std::function<ThreadX::Uint(ThreadX::Native::LX_NOR_FLASH *norFlashPtr)> initialiseCallback;
        std::function<ThreadX::Uint(ThreadX::Native::LX_NOR_FLASH *norFlashPtr, ThreadX::Ulong *flashAddress,
                                    ThreadX::Ulong *destination, ThreadX::Ulong words)>
            readCallback;
        std::function<ThreadX::Uint(ThreadX::Native::LX_NOR_FLASH *norFlashPtr, ThreadX::Ulong *flashAddress,
                                    ThreadX::Ulong *source, ThreadX::Ulong words)>
            writeCallback;
        std::function<ThreadX::Uint(
            ThreadX::Native::LX_NOR_FLASH *norFlashPtr, ThreadX::Ulong block, ThreadX::Ulong eraseCount)>
            eraseBlockCallback;
        std::function<ThreadX::Uint(ThreadX::Native::LX_NOR_FLASH *norFlashPtr, ThreadX::Ulong block)>
            verifyErasedBlockCallback;
        std::function<void(ThreadX::Native::LX_NOR_FLASH *norFlashPtr, ThreadX::Uint errorCode)> systemErrorCallback;
    };

    static constexpr ThreadX::Uint m_sectorSizeInWord = 512 / ThreadX::wordSize; // LX_NOR_SECTOR_SIZE;
    static constexpr FileX::SectorSize sectorSize();

    NorFlashBase(const Driver &driver);
    ~NorFlashBase();

    ThreadX::Ulong formatSize() const;
    Error open();
    Error close();
    Error defragment(const ThreadX::Uint numberOfBlocks = 0);
    Error readSector(const ThreadX::Ulong sectorNumber, std::span<ThreadX::Ulong, m_sectorSizeInWord> sectorData);
    Error releaseSector(const ThreadX::Ulong sectorNumber);
    Error writeSector(const ThreadX::Ulong sectorNumber, std::span<ThreadX::Ulong, m_sectorSizeInWord> sectorData);

  protected:
    static inline std::atomic_flag m_initialised = ATOMIC_FLAG_INIT;

    void init(std::span<ThreadX::Ulong> extendedCacheMemory, const ThreadX::Ulong storageSize,
              const ThreadX::Ulong blockSize, const ThreadX::Ulong baseAddress);

  private:
    struct DriverCallbacks
    {
        static ThreadX::Uint initialise(ThreadX::Native::LX_NOR_FLASH *norFlashPtr);
        static ThreadX::Uint read(ThreadX::Native::LX_NOR_FLASH *norFlashPtr, ThreadX::Ulong *flashAddress,
                                  ThreadX::Ulong *destination, ThreadX::Ulong words);
        static ThreadX::Uint write(ThreadX::Native::LX_NOR_FLASH *norFlashPtr, ThreadX::Ulong *flashAddress,
                                   ThreadX::Ulong *source, ThreadX::Ulong words);
        static ThreadX::Uint eraseBlock(
            ThreadX::Native::LX_NOR_FLASH *norFlashPtr, ThreadX::Ulong block, ThreadX::Ulong eraseCount);
        static ThreadX::Uint verifyErasedBlock(ThreadX::Native::LX_NOR_FLASH *norFlashPtr, ThreadX::Ulong block);
        static ThreadX::Uint systemError(ThreadX::Native::LX_NOR_FLASH *norFlashPtr, ThreadX::Uint errorCode);
    };

    Driver m_driver;
    std::array<ThreadX::Ulong, m_sectorSizeInWord> m_sectorBuffer{};
};

constexpr FileX::SectorSize NorFlashBase::sectorSize()
{
    return FileX::SectorSize{m_sectorSizeInWord * ThreadX::wordSize};
}

template <ThreadX::Ulong CacheSize = 0> class NorFlash : public NorFlashBase
{
  public:
    NorFlash(const ThreadX::Ulong storageSize, const ThreadX::Ulong blockSize, const Driver &driver,
             const ThreadX::Ulong baseAddress = 0);

  private:
    using NorFlashBase::init;
    using NorFlashBase::m_initialised;

    std::array<ThreadX::Ulong, CacheSize / ThreadX::wordSize> m_extendedCacheMemory{};
};

template <ThreadX::Ulong CacheSize>
NorFlash<CacheSize>::NorFlash(const ThreadX::Ulong storageSize, const ThreadX::Ulong blockSize, const Driver &driver,
                              const ThreadX::Ulong baseAddress)
    : NorFlashBase{driver}
{
    static_assert(CacheSize % (m_sectorSizeInWord * ThreadX::wordSize) == 0);

    init(m_extendedCacheMemory, storageSize, blockSize, baseAddress);
}
} // namespace LevelX