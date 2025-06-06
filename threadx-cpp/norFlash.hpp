#pragma once

#include "lxCommon.hpp"
#include <array>
#include <cassert> // Added missing include for assert
#include <mutex>
#include <span>
#include <utility>

namespace LevelX
{
inline constexpr ThreadX::Uint norSectorSizeInWord{LX_NATIVE_NOR_SECTOR_SIZE};
inline constexpr ThreadX::Ulong norSectorSize{norSectorSizeInWord * ThreadX::wordSize};
inline constexpr ThreadX::Ulong norBootSector{0};

template <typename T>
concept NorFlashDriver =
    requires(T t, ThreadX::Ulong *const flashAddress, const std::span<ThreadX::Ulong> destination, const std::span<const ThreadX::Ulong> source,
             const ThreadX::Ulong block, const ThreadX::Ulong blockCount, const ThreadX::Uint errorCode) {
        { t.initialise() } -> std::same_as<Error>;
        { t.read(flashAddress, destination) } -> std::same_as<Error>;
        // LevelX relies on the driver to verify that the write sector was successful. This is typically done by reading back the programmed value to ensure it
        // matches the requested value to be written.
        { t.write(flashAddress, source) } -> std::same_as<Error>;
        // LevelX relies on the driver to examine all bytes of the block to ensure they are erased (contain all ones).
        { t.eraseBlock(block, blockCount) } -> std::same_as<Error>;
        // LevelX relies on the driver to examine all bytes of the specified to ensure they are erased (contain all ones).
        { t.verifyErasedBlock(block) } -> std::same_as<Error>;
        { t.systemError(errorCode) } -> std::same_as<Error>;
    };

struct NorSectorMetadata
{
    ThreadX::Ulong logicalSector : 29; // Logical sector mapped to this physical sector—when not all ones.
    ThreadX::Ulong writeComplete : 1;  // Mapping entry write is complete when this bit is 0
    ThreadX::Ulong obsoleteFlag : 1;   // Obsolete flag. When clear, this mapping is either obsolete or is in the process of becoming obsolete.
    ThreadX::Ulong validFlag : 1;      // Valid flag. When set and logical sector not all ones indicates mapping is valid
};

struct NorPhysicalSector
{
    ThreadX::Ulong memory[LevelX::norSectorSizeInWord];
};

class NorFlashBase
{
  protected:
    static inline std::once_flag m_initialisedFlag{};
};

template <ThreadX::Uint BlockSectors, NorFlashDriver Driver, ThreadX::Uint CacheSectors = 0>
class NorFlash : ThreadX::Native::LX_NOR_FLASH, NorFlashBase
{
    static_assert(BlockSectors >= 2 and BlockSectors <= 122);

  public:
    static constexpr auto usableSectorsPerBlock{BlockSectors - 1};
    static constexpr auto freeBitmapWords{((usableSectorsPerBlock - 1) / 32) + 1};
    static constexpr auto unusedMetadataWordsPerBlock{LevelX::norSectorSizeInWord - (3 + freeBitmapWords + usableSectorsPerBlock)};

    struct Block
    {
        ThreadX::Ulong eraseCount;
        ThreadX::Ulong minLogSector;
        ThreadX::Ulong maxLogSector;
        ThreadX::Ulong freeBitMap[freeBitmapWords];
        NorSectorMetadata sectorMetadata[usableSectorsPerBlock];
        ThreadX::Ulong unusedWords[unusedMetadataWordsPerBlock];
        NorPhysicalSector physicalSectors[usableSectorsPerBlock];
    };

    static constexpr auto sectorSize() -> FileX::MediaSectorSize;

    explicit NorFlash(Driver &driver, const ThreadX::Ulong storageSize, const ThreadX::Ulong baseAddress = 0);
    ~NorFlash();

    [[nodiscard]] auto mediaFormatSize() const -> ThreadX::Ulong;

    auto open() -> Error;

    auto close() -> Error;

    auto defragment() -> Error;

    auto defragment(const ThreadX::Uint numberOfBlocks) -> Error;

    // sectorData must be word aligned. Uchar type because media driver pointers are char*.
    auto readSector(const ThreadX::Ulong logicalSector, std::span<ThreadX::Ulong, norSectorSizeInWord> sectorData) -> Error;

    // sectorData must be word aligned. Uchar type because media driver pointers are char*.
    auto writeSector(const ThreadX::Ulong logicalSector, const std::span<ThreadX::Ulong, norSectorSizeInWord> sectorData) -> Error;

    auto releaseSector(const ThreadX::Ulong logicalSector) -> Error;

  private:
    struct DriverCallback
    {
        static auto initialise(ThreadX::Native::LX_NOR_FLASH *norFlashPtr) -> ThreadX::Uint;
        static auto read(ThreadX::Native::LX_NOR_FLASH *norFlashPtr, ThreadX::Ulong *flashAddress, ThreadX::Ulong *destination, ThreadX::Ulong words)
            -> ThreadX::Uint;
        static auto write(ThreadX::Native::LX_NOR_FLASH *norFlashPtr, ThreadX::Ulong *flashAddress, ThreadX::Ulong *source, ThreadX::Ulong words)
            -> ThreadX::Uint;
        static auto eraseBlock(ThreadX::Native::LX_NOR_FLASH *norFlashPtr, ThreadX::Ulong block, ThreadX::Ulong eraseCount) -> ThreadX::Uint;
        static auto verifyErasedBlock(ThreadX::Native::LX_NOR_FLASH *norFlashPtr, ThreadX::Ulong block) -> ThreadX::Uint;
        static auto systemError(ThreadX::Native::LX_NOR_FLASH *norFlashPtr, ThreadX::Uint errorCode) -> ThreadX::Uint;
    };

    static constexpr ThreadX::Ulong m_blockSize{BlockSectors * norSectorSize};

    Driver &m_driver;
    const ThreadX::Ulong m_storageSize;
    const ThreadX::Ulong m_baseAddress;
    std::array<ThreadX::Ulong, norSectorSizeInWord> m_sectorBuffer{};
    alignas(ThreadX::Ulong) std::array<std::byte, CacheSectors * norSectorSize> m_extendedCacheMemory{};
};

template <ThreadX::Uint BlockSectors, NorFlashDriver Driver, ThreadX::Uint CacheSectors>
constexpr auto NorFlash<BlockSectors, Driver, CacheSectors>::sectorSize() -> FileX::MediaSectorSize
{
    return static_cast<FileX::MediaSectorSize>(norSectorSize);
}

template <ThreadX::Uint BlockSectors, NorFlashDriver Driver, ThreadX::Uint CacheSectors>
NorFlash<BlockSectors, Driver, CacheSectors>::NorFlash(Driver &driver, const ThreadX::Ulong storageSize, const ThreadX::Ulong baseAddress)
    : ThreadX::Native::LX_NOR_FLASH{}, m_driver{driver}, m_storageSize{storageSize}, m_baseAddress{baseAddress}
{
    assert(storageSize % (BlockSectors * norSectorSize) == 0);

    std::call_once(m_initialisedFlag, []() { ThreadX::Native::lx_nor_flash_initialize(); });
}

template <ThreadX::Uint BlockSectors, NorFlashDriver Driver, ThreadX::Uint CacheSectors>
NorFlash<BlockSectors, Driver, CacheSectors>::~NorFlash()
{
    [[maybe_unused]] auto error{close()};
    assert(error == Error::success);
}

template <ThreadX::Uint BlockSectors, NorFlashDriver Driver, ThreadX::Uint CacheSectors>
auto NorFlash<BlockSectors, Driver, CacheSectors>::mediaFormatSize() const -> ThreadX::Ulong
{
    assert(lx_nor_flash_total_blocks > 1);
    return ThreadX::Ulong{(lx_nor_flash_total_blocks - 1) * (lx_nor_flash_words_per_block * ThreadX::wordSize)};
}

template <ThreadX::Uint BlockSectors, NorFlashDriver Driver, ThreadX::Uint CacheSectors>
auto NorFlash<BlockSectors, Driver, CacheSectors>::open() -> Error
{
    Error error{lx_nor_flash_open(this, const_cast<char *>("nor flash"), DriverCallback::initialise)};
    if (error != Error::success)
    {
        return error;
    }

    if constexpr (CacheSectors > 0)
    {
        return Error{lx_nor_flash_extended_cache_enable(this, m_extendedCacheMemory.data(), m_extendedCacheMemory.size())};
    }

    return Error::success;
}

template <ThreadX::Uint BlockSectors, NorFlashDriver Driver, ThreadX::Uint CacheSectors>
auto NorFlash<BlockSectors, Driver, CacheSectors>::close() -> Error
{
    return Error{lx_nor_flash_close(this)};
}

template <ThreadX::Uint BlockSectors, NorFlashDriver Driver, ThreadX::Uint CacheSectors>
auto NorFlash<BlockSectors, Driver, CacheSectors>::defragment() -> Error
{
    return Error{lx_nor_flash_defragment(this)};
}

template <ThreadX::Uint BlockSectors, NorFlashDriver Driver, ThreadX::Uint CacheSectors>
auto NorFlash<BlockSectors, Driver, CacheSectors>::defragment(const ThreadX::Uint numberOfBlocks) -> Error
{
    return Error{lx_nor_flash_partial_defragment(this, numberOfBlocks)};
}

template <ThreadX::Uint BlockSectors, NorFlashDriver Driver, ThreadX::Uint CacheSectors>
auto NorFlash<BlockSectors, Driver, CacheSectors>::readSector(const ThreadX::Ulong logicalSector, std::span<ThreadX::Ulong, norSectorSizeInWord> sectorData)
    -> Error
{
    return Error{lx_nor_flash_sector_read(this, logicalSector, sectorData.data())};
}

template <ThreadX::Uint BlockSectors, NorFlashDriver Driver, ThreadX::Uint CacheSectors>
auto NorFlash<BlockSectors, Driver, CacheSectors>::writeSector(const ThreadX::Ulong logicalSector, std::span<ThreadX::Ulong, norSectorSizeInWord> sectorData)
    -> Error
{
    return Error{lx_nor_flash_sector_write(this, logicalSector, sectorData.data())};
}

template <ThreadX::Uint BlockSectors, NorFlashDriver Driver, ThreadX::Uint CacheSectors>
auto NorFlash<BlockSectors, Driver, CacheSectors>::releaseSector(const ThreadX::Ulong logicalSector) -> Error
{
    return Error{lx_nor_flash_sector_release(this, logicalSector)};
}

template <ThreadX::Uint BlockSectors, NorFlashDriver Driver, ThreadX::Uint CacheSectors>
auto NorFlash<BlockSectors, Driver, CacheSectors>::DriverCallback::initialise(ThreadX::Native::LX_NOR_FLASH *norFlashPtr) -> ThreadX::Uint
{
    auto &norFlash{static_cast<NorFlash &>(*norFlashPtr)};

    norFlash.lx_nor_flash_base_address = reinterpret_cast<ThreadX::Ulong *>(norFlash.m_baseAddress);
    norFlash.lx_nor_flash_total_blocks = norFlash.m_storageSize / norFlash.m_blockSize;
    norFlash.lx_nor_flash_words_per_block = norFlash.m_blockSize / ThreadX::wordSize;
    norFlash.lx_nor_flash_sector_buffer = norFlash.m_sectorBuffer.data();
    norFlash.lx_nor_flash_driver_read = DriverCallback::read;
    norFlash.lx_nor_flash_driver_write = DriverCallback::write;
    norFlash.lx_nor_flash_driver_block_erase = DriverCallback::eraseBlock;
    norFlash.lx_nor_flash_driver_block_erased_verify = DriverCallback::verifyErasedBlock;
    norFlash.lx_nor_flash_driver_system_error = DriverCallback::systemError;

    return std::to_underlying(norFlash.m_driver.initialise());
}

template <ThreadX::Uint BlockSectors, NorFlashDriver Driver, ThreadX::Uint CacheSectors>
auto NorFlash<BlockSectors, Driver, CacheSectors>::DriverCallback::read(ThreadX::Native::LX_NOR_FLASH *norFlashPtr, ThreadX::Ulong *flashAddress,
                                                                        ThreadX::Ulong *destination, ThreadX::Ulong words) -> ThreadX::Uint
{
    auto &norFlash{static_cast<NorFlash &>(*norFlashPtr)};
    return std::to_underlying(norFlash.m_driver.read(flashAddress, {destination, words}));
}

template <ThreadX::Uint BlockSectors, NorFlashDriver Driver, ThreadX::Uint CacheSectors>
auto NorFlash<BlockSectors, Driver, CacheSectors>::DriverCallback::write(ThreadX::Native::LX_NOR_FLASH *norFlashPtr, ThreadX::Ulong *flashAddress,
                                                                         ThreadX::Ulong *source, ThreadX::Ulong words) -> ThreadX::Uint
{
    auto &norFlash{static_cast<NorFlash &>(*norFlashPtr)};
    return std::to_underlying(norFlash.m_driver.write(flashAddress, {source, words}));
}

template <ThreadX::Uint BlockSectors, NorFlashDriver Driver, ThreadX::Uint CacheSectors>
auto NorFlash<BlockSectors, Driver, CacheSectors>::DriverCallback::eraseBlock(ThreadX::Native::LX_NOR_FLASH *norFlashPtr, ThreadX::Ulong block,
                                                                              ThreadX::Ulong blockCount) -> ThreadX::Uint
{
    auto &norFlash{static_cast<NorFlash &>(*norFlashPtr)};
    return std::to_underlying(norFlash.m_driver.eraseBlock(block, blockCount));
}

template <ThreadX::Uint BlockSectors, NorFlashDriver Driver, ThreadX::Uint CacheSectors>
auto NorFlash<BlockSectors, Driver, CacheSectors>::DriverCallback::verifyErasedBlock(ThreadX::Native::LX_NOR_FLASH *norFlashPtr, ThreadX::Ulong block)
    -> ThreadX::Uint
{
    auto &norFlash{static_cast<NorFlash &>(*norFlashPtr)};
    return std::to_underlying(norFlash.m_driver.verifyErasedBlock(block));
}

template <ThreadX::Uint BlockSectors, NorFlashDriver Driver, ThreadX::Uint CacheSectors>
auto NorFlash<BlockSectors, Driver, CacheSectors>::DriverCallback::systemError(ThreadX::Native::LX_NOR_FLASH *norFlashPtr, ThreadX::Uint errorCode)
    -> ThreadX::Uint
{
    auto &norFlash{static_cast<NorFlash &>(*norFlashPtr)};
    return std::to_underlying(norFlash.m_driver.systemError(errorCode));
}
} // namespace LevelX
