#pragma once

#include "lxCommon.hpp"
#include <atomic>
#include <span>
#include <utility>

#if 0
namespace LevelX
{
enum class NandPageSize : ThreadX::Ulong
{
    aQuarterOfKilobyte = 256,
    halfAKilobyte = aQuarterOfKilobyte * 2,
    twoKiloBytes = halfAKilobyte * 4,
    fourKiloBytes = twoKiloBytes * 2,
    eightKiloBytes = fourKiloBytes * 2
};

struct NandSpareDataInfo
{
    ThreadX::Ulong data1Offset;
    ThreadX::Ulong data1Length;
    ThreadX::Ulong data2Offset;
    ThreadX::Ulong data2Length;
};

inline constexpr ThreadX::Ulong nandBootSector{0};

#define BAD_BLOCK_POSITION 0  /*      0 is the bad block byte postion                         */
#define EXTRA_BYTE_POSITION 0 /*      0 is the extra bytes starting byte postion              */
#define ECC_BYTE_POSITION 8   /*      8 is the ECC starting byte position                     */

/* Definition of the spare area is relative to the block size of the NAND part and perhaps manufactures of the NAND part. 
   Here are some common definitions:
   
   256 Byte Block
   
        Bytes               Meaning
        
        0,1,2           ECC bytes
        3,4,6,7         Extra
        5               Bad block flag
        
    512 Byte Block
    
        Bytes               Meaning
        
        0,1,2,3,6,7     ECC bytes
        8-15            Extra
        5               Bad block flag
    
    2048 Byte Block
    
        Bytes               Meaning
        
        0               Bad block flag
        2-39            Extra
        40-63           ECC bytes
*/

class NandFlashBase
{
  protected:
    static inline std::once_flag m_initialisedFlag{};
};

template <ThreadX::Ulong Blocks, ThreadX::Ulong BlockPages, ThreadX::Ulong PageSize>
class NandFlash : ThreadX::Native::LX_NAND_FLASH, NandFlashBase
{
    static_assert(BlockPages >= 2);

    using UlongPair = std::pair<ThreadX::Ulong, ThreadX::Ulong>;

  public:
    static constexpr auto pageDataAndSpareSizes();

    struct PhysicalPage
    {
        ThreadX::Ulong memory[pageDataAndSpareSizes().first / ThreadX::wordSize];
        ThreadX::Uchar spare[pageDataAndSpareSizes().second];
    };

    struct Block
    {
        PhysicalPage physicalPages[BlockPages];
    };

    struct BlockDiag
    {
        ThreadX::Ulong erases;
        ThreadX::Ulong pageWrites[BlockPages];
        ThreadX::Ulong maxPageWrites[BlockPages];
    };

    static constexpr ThreadX::Uint eccSize{
        3 * (pageDataAndSpareSizes().first / std::to_underlying(NandPageSize::aQuarterOfKilobyte))};

    static constexpr auto blockPages();
    static constexpr auto pageDataSize();
    static constexpr auto spareBytesSize();
    static constexpr auto sectorSize();
    NandFlash(const NandSpareDataInfo &spareDataInfo);

    auto mediaFormatSize() const;
    auto open();
    auto format();
    auto readSectors(const ThreadX::Ulong logicalSector, const std::span<ThreadX::Ulong> sectorData);
    auto releaseSectors(const ThreadX::Ulong logicalSector, const ThreadX::Ulong sectorCount = 1);
    auto writeSectors(const ThreadX::Ulong logicalSector, const std::span<ThreadX::Ulong> sectorData);
    auto computePageEcc(const std::span<ThreadX::Uchar, pageDataAndSpareSizes().first / ThreadX::wordSize> pageBuffer,
                        std::span<ThreadX::Uchar, eccSize> ecc);
    auto checkPageEcc(const std::span<ThreadX::Uchar, pageDataAndSpareSizes().first / ThreadX::wordSize> pageBuffer,
                      const std::span<ThreadX::Uchar, eccSize> ecc);
    auto close();

    virtual Error initialiseCallback();
    virtual Error readCallback(
        ThreadX::Ulong block, ThreadX::Ulong page, ThreadX::Ulong *destination, ThreadX::Ulong words) = 0;
    virtual Error writeCallback(
        ThreadX::Ulong block, ThreadX::Ulong page, ThreadX::Ulong *source, ThreadX::Ulong words) = 0;
    virtual Error readPagesCallback(ThreadX::Ulong block, ThreadX::Ulong page, ThreadX::Uchar *mainBuffer,
                                    ThreadX::Uchar *spareBuffer, ThreadX::Ulong pages) = 0;
    virtual Error writePagesCallback(ThreadX::Ulong block, ThreadX::Ulong page, ThreadX::Uchar *mainBuffer,
                                     ThreadX::Uchar *spareBuffer, ThreadX::Ulong pages) = 0;
    virtual Error copyPagesCallback(
        ThreadX::Ulong sourceBlock, ThreadX::Ulong sourcePage, ThreadX::Ulong destinationBlock,
        ThreadX::Ulong destinationPage, ThreadX::Ulong pages, ThreadX::Uchar *dataBuffer) = 0;
    virtual Error eraseBlockCallback(ThreadX::Ulong block, ThreadX::Ulong eraseCount) = 0;
    virtual Error verifyErasedBlockCallback(ThreadX::Ulong block) = 0;
    virtual Error verifyErasedPageCallback(ThreadX::Ulong block, ThreadX::Ulong page) = 0;
    virtual Error getBlockStatusCallback(ThreadX::Ulong block, ThreadX::Uchar *badBlockFlag) = 0;
    virtual Error setBlockStatusCallback(ThreadX::Ulong block, ThreadX::Uchar badBlockFlag) = 0;
    virtual Error getExtraBytesCallback(
        ThreadX::Ulong block, ThreadX::Ulong page, ThreadX::Uchar *destination, ThreadX::Uint size) = 0;
    virtual Error setExtraBytesCallback(
        ThreadX::Ulong block, ThreadX::Ulong page, ThreadX::Uchar *source, ThreadX::Uint size) = 0;
    virtual Error systemErrorCallback(ThreadX::Uint errorCode, ThreadX::Ulong block, ThreadX::Ulong page);

  protected:
    ~NandFlash();

  private:
    struct DriverCallback
    {
        static auto initialise(ThreadX::Native::LX_NAND_FLASH *nandFlashPtr);
        static auto read(ThreadX::Native::LX_NAND_FLASH *nandFlashPtr, ThreadX::Ulong block, ThreadX::Ulong page,
                         ThreadX::Ulong *destination, ThreadX::Ulong words);
        static auto write(ThreadX::Native::LX_NAND_FLASH *nandFlashPtr, ThreadX::Ulong block, ThreadX::Ulong page,
                          ThreadX::Ulong *source, ThreadX::Ulong words);
        static auto readPages(ThreadX::Native::LX_NAND_FLASH *nandFlashPtr, ThreadX::Ulong block, ThreadX::Ulong page,
                              ThreadX::Uchar *mainBuffer, ThreadX::Uchar *spareBuffer, ThreadX::Ulong pages);
        static auto writePages(ThreadX::Native::LX_NAND_FLASH *nandFlashPtr, ThreadX::Ulong block, ThreadX::Ulong page,
                               ThreadX::Uchar *mainBuffer, ThreadX::Uchar *spareBuffer, ThreadX::Ulong pages);
        static auto copyPages(ThreadX::Native::LX_NAND_FLASH *nandFlashPtr, ThreadX::Ulong sourceBlock,
                              ThreadX::Ulong sourcePage, ThreadX::Ulong destinationBlock,
                              ThreadX::Ulong destinationPage, ThreadX::Ulong pages, ThreadX::Uchar *dataBuffer);
        static auto eraseBlock(
            ThreadX::Native::LX_NAND_FLASH *nandFlashPtr, ThreadX::Ulong block, ThreadX::Ulong eraseCount);
        static auto verifyErasedBlock(ThreadX::Native::LX_NAND_FLASH *nandFlashPtr, ThreadX::Ulong block);
        static auto verifyErasedPage(
            ThreadX::Native::LX_NAND_FLASH *nandFlashPtr, ThreadX::Ulong block, ThreadX::Ulong page);
        static auto getBlockStatus(
            ThreadX::Native::LX_NAND_FLASH *nandFlashPtr, ThreadX::Ulong block, ThreadX::Uchar *badBlockFlag);
        static auto setBlockStatus(
            ThreadX::Native::LX_NAND_FLASH *nandFlashPtr, ThreadX::Ulong block, ThreadX::Uchar badBlockFlag);
        static auto getExtraBytes(ThreadX::Native::LX_NAND_FLASH *nandFlashPtr, ThreadX::Ulong block,
                                  ThreadX::Ulong page, ThreadX::Uchar *destination, ThreadX::Uint size);
        static auto setExtraBytes(ThreadX::Native::LX_NAND_FLASH *nandFlashPtr, ThreadX::Ulong block,
                                  ThreadX::Ulong page, ThreadX::Uchar *source, ThreadX::Uint size);
        static auto systemError(ThreadX::Native::LX_NAND_FLASH *nandFlashPtr, ThreadX::Uint errorCode,
                                ThreadX::Ulong block, ThreadX::Ulong page);
    };

    NandSpareDataInfo m_spareDataInfo;
    alignas(Ulong) std::array<ThreadX::Ulong, (2 * Blocks + 3 * pageDataSize() + 2 * PageSize)> workingMemory{};
};

template <ThreadX::Ulong Blocks, ThreadX::Ulong BlockPages, ThreadX::Ulong PageSize>
constexpr auto NandFlash<Blocks, BlockPages, PageSize>::blockPages()
{
    return BlockPages;
}

template <ThreadX::Ulong Blocks, ThreadX::Ulong BlockPages, ThreadX::Ulong PageSize>
constexpr auto NandFlash<Blocks, BlockPages, PageSize>::pageDataSize()
{
    return pageDataAndSpareSizes().first;
}

template <ThreadX::Ulong Blocks, ThreadX::Ulong BlockPages, ThreadX::Ulong PageSize>
constexpr auto NandFlash<Blocks, BlockPages, PageSize>::spareBytesSize()
{
    return pageDataAndSpareSizes().second;
}

template <ThreadX::Ulong Blocks, ThreadX::Ulong BlockPages, ThreadX::Ulong PageSize>
constexpr auto NandFlash<Blocks, BlockPages, PageSize>::sectorSize()
{
    return static_cast<FileX::MediaSectorSize>(pageDataAndSpareSizes().first);
}

template <ThreadX::Ulong Blocks, ThreadX::Ulong BlockPages, ThreadX::Ulong PageSize>
NandFlash<Blocks, BlockPages, PageSize>::NandFlash(const NandSpareDataInfo &spareDataInfo)
    : ThreadX::Native::LX_NAND_FLASH{}, m_spareDataInfo{spareDataInfo}
{
    std::call_once(m_initialisedFlag, []() { ThreadX::Native::lx_nand_flash_initialize(); });
}

template <ThreadX::Ulong Blocks, ThreadX::Ulong BlockPages, ThreadX::Ulong PageSize>
NandFlash<Blocks, BlockPages, PageSize>::~NandFlash()
{
    [[maybe_unused]] auto error{close()};
    assert(error == Error::success);
}

template <ThreadX::Ulong Blocks, ThreadX::Ulong BlockPages, ThreadX::Ulong PageSize>
auto NandFlash<Blocks, BlockPages, PageSize>::mediaFormatSize() const
{
    assert(Blocks > 1);
    return ThreadX::Ulong{(Blocks - 1) * (lx_nand_flash_words_per_block * ThreadX::wordSize)};
}

template <ThreadX::Ulong Blocks, ThreadX::Ulong BlockPages, ThreadX::Ulong PageSize>
auto NandFlash<Blocks, BlockPages, PageSize>::open()
{
    return Error{lx_nand_flash_open(this, const_cast<char *>("nand flash"), DriverCallback::initialise,
                                    workingMemory.data(), workingMemory.size())};
}

template <ThreadX::Ulong Blocks, ThreadX::Ulong BlockPages, ThreadX::Ulong PageSize>
auto NandFlash<Blocks, BlockPages, PageSize>::format()
{
    return Error{lx_nand_flash_format(this, const_cast<char *>("nand flash"), DriverCallback::initialise,
                                      workingMemory.data(), workingMemory.size())};
}

template <ThreadX::Ulong Blocks, ThreadX::Ulong BlockPages, ThreadX::Ulong PageSize>
auto NandFlash<Blocks, BlockPages, PageSize>::close()
{
    return Error{lx_nand_flash_close(this)};
}

template <ThreadX::Ulong Blocks, ThreadX::Ulong BlockPages, ThreadX::Ulong PageSize>
auto NandFlash<Blocks, BlockPages, PageSize>::readSectors(
    const ThreadX::Ulong logicalSector, const std::span<ThreadX::Ulong> sectorData)
{
    assert(sectorData.size_bytes() % std::to_underlying(sectorSize()) == 0);

    return Error{lx_nand_flash_sectors_read(
        this, logicalSector, sectorData.data(), sectorData.size_bytes() / std::to_underlying(sectorSize()))};
}

template <ThreadX::Ulong Blocks, ThreadX::Ulong BlockPages, ThreadX::Ulong PageSize>
auto NandFlash<Blocks, BlockPages, PageSize>::releaseSectors(
    const ThreadX::Ulong logicalSector, const ThreadX::Ulong sectorCount)
{
    return Error{lx_nand_flash_sectors_release(this, logicalSector, sectorCount)};
}

template <ThreadX::Ulong Blocks, ThreadX::Ulong BlockPages, ThreadX::Ulong PageSize>
auto NandFlash<Blocks, BlockPages, PageSize>::writeSectors(
    const ThreadX::Ulong logicalSector, const std::span<ThreadX::Ulong> sectorData)
{
    assert(sectorData.size_bytes() % std::to_underlying(sectorSize()) == 0);

    return Error{lx_nand_flash_sectors_write(
        this, logicalSector, sectorData.data(), sectorData.size_bytes() / std::to_underlying(sectorSize()))};
}

template <ThreadX::Ulong Blocks, ThreadX::Ulong BlockPages, ThreadX::Ulong PageSize>
auto NandFlash<Blocks, BlockPages, PageSize>::computePageEcc(
    const std::span<ThreadX::Uchar, pageDataAndSpareSizes().first / ThreadX::wordSize> pageBuffer,
    std::span<ThreadX::Uchar, eccSize> ecc)
{
    return Error{lx_nand_flash_page_ecc_compute(this, pageBuffer.data(), ecc.data())};
}

template <ThreadX::Ulong Blocks, ThreadX::Ulong BlockPages, ThreadX::Ulong PageSize>
auto NandFlash<Blocks, BlockPages, PageSize>::checkPageEcc(
    const std::span<ThreadX::Uchar, pageDataAndSpareSizes().first / ThreadX::wordSize> pageBuffer,
    const std::span<ThreadX::Uchar, eccSize> ecc)
{
    return Error{lx_nand_flash_page_ecc_check(this, pageBuffer.data(), ecc.data())};
}

template <ThreadX::Ulong Blocks, ThreadX::Ulong BlockPages, ThreadX::Ulong PageSize>
Error NandFlash<Blocks, BlockPages, PageSize>::initialiseCallback()
{
    return Error::success;
}

template <ThreadX::Ulong Blocks, ThreadX::Ulong BlockPages, ThreadX::Ulong PageSize>
Error NandFlash<Blocks, BlockPages, PageSize>::systemErrorCallback(
    [[maybe_unused]] const ThreadX::Uint errorCode, [[maybe_unused]] const ThreadX::Ulong block,
    [[maybe_unused]] const ThreadX::Ulong page)
{
    return Error::success;
}

template <ThreadX::Ulong Blocks, ThreadX::Ulong BlockPages, ThreadX::Ulong PageSize>
auto NandFlash<Blocks, BlockPages, PageSize>::DriverCallback::initialise(ThreadX::Native::LX_NAND_FLASH *nandFlashPtr)
{
    auto &nandFlash{static_cast<NandFlash &>(*nandFlashPtr)};

    nandFlash.lx_nand_flash_total_blocks = Blocks;
    nandFlash.lx_nand_flash_pages_per_block = BlockPages;

    auto pageSizes{nandFlash.pageDataAndSpareSizes()};
    nandFlash.lx_nand_flash_bytes_per_page = pageSizes.first;
    nandFlash.lx_nand_flash_spare_total_length = pageSizes.second;

    nandFlash.lx_nand_flash_spare_data1_offset = nandFlash.m_spareDataInfo.data1Offset;
    nandFlash.lx_nand_flash_spare_data1_length = nandFlash.m_spareDataInfo.data1Length;

    nandFlash.lx_nand_flash_spare_data2_offset = nandFlash.m_spareDataInfo.data2Offset;
    nandFlash.lx_nand_flash_spare_data2_length = nandFlash.m_spareDataInfo.data2Length;

    nandFlash.lx_nand_flash_driver_read = DriverCallback::read;
    nandFlash.lx_nand_flash_driver_write = DriverCallback::write;
    nandFlash.lx_nand_flash_driver_pages_read = DriverCallback::readPages;
    nandFlash.lx_nand_flash_driver_pages_write = DriverCallback::writePages;
    nandFlash.lx_nand_flash_driver_pages_copy = DriverCallback::copyPages;
    nandFlash.lx_nand_flash_driver_block_erase = DriverCallback::eraseBlock;
    nandFlash.lx_nand_flash_driver_block_erased_verify = DriverCallback::verifyErasedBlock;
    nandFlash.lx_nand_flash_driver_page_erased_verify = DriverCallback::verifyErasedPage;
    nandFlash.lx_nand_flash_driver_block_status_get = DriverCallback::getBlockStatus;
    nandFlash.lx_nand_flash_driver_block_status_set = DriverCallback::setBlockStatus;
    nandFlash.lx_nand_flash_driver_extra_bytes_get = DriverCallback::getExtraBytes;
    nandFlash.lx_nand_flash_driver_extra_bytes_set = DriverCallback::setExtraBytes;
    nandFlash.lx_nand_flash_driver_system_error = DriverCallback::systemError;

    return std::to_underlying(nandFlash.initialiseCallback());
}

template <ThreadX::Ulong Blocks, ThreadX::Ulong BlockPages, ThreadX::Ulong PageSize>
constexpr auto NandFlash<Blocks, BlockPages, PageSize>::pageDataAndSpareSizes()
{
    ThreadX::Ulong pageDataSize{4096};
    while (PageSize / pageDataSize == 0)
    {
        pageDataSize /= 2;
    }

    return UlongPair{pageDataSize, PageSize % pageDataSize};
};

template <ThreadX::Ulong Blocks, ThreadX::Ulong BlockPages, ThreadX::Ulong PageSize>
auto NandFlash<Blocks, BlockPages, PageSize>::DriverCallback::read(
    ThreadX::Native::LX_NAND_FLASH *nandFlashPtr, ThreadX::Ulong block, ThreadX::Ulong page,
    ThreadX::Ulong *destination, ThreadX::Ulong words)
{
    auto &nandFlash{static_cast<NandFlash &>(*nandFlashPtr)};
    return std::to_underlying(nandFlash.readCallback(block, page, destination, words));
}

template <ThreadX::Ulong Blocks, ThreadX::Ulong BlockPages, ThreadX::Ulong PageSize>
auto NandFlash<Blocks, BlockPages, PageSize>::DriverCallback::write(
    ThreadX::Native::LX_NAND_FLASH *nandFlashPtr, ThreadX::Ulong block, ThreadX::Ulong page, ThreadX::Ulong *source,
    ThreadX::Ulong words)
{
    auto &nandFlash{static_cast<NandFlash &>(*nandFlashPtr)};
    return std::to_underlying(nandFlash.writeCallback(block, page, source, words));
}

template <ThreadX::Ulong Blocks, ThreadX::Ulong BlockPages, ThreadX::Ulong PageSize>
auto NandFlash<Blocks, BlockPages, PageSize>::DriverCallback::readPages(
    ThreadX::Native::LX_NAND_FLASH *nandFlashPtr, ThreadX::Ulong block, ThreadX::Ulong page, ThreadX::Uchar *mainBuffer,
    ThreadX::Uchar *spareBuffer, ThreadX::Ulong pages)
{
    auto &nandFlash{static_cast<NandFlash &>(*nandFlashPtr)};
    return std::to_underlying(nandFlash.readPagesCallback(block, page, mainBuffer, spareBuffer, pages));
}

template <ThreadX::Ulong Blocks, ThreadX::Ulong BlockPages, ThreadX::Ulong PageSize>
auto NandFlash<Blocks, BlockPages, PageSize>::DriverCallback::writePages(
    ThreadX::Native::LX_NAND_FLASH *nandFlashPtr, ThreadX::Ulong block, ThreadX::Ulong page, ThreadX::Uchar *mainBuffer,
    ThreadX::Uchar *spareBuffer, ThreadX::Ulong pages)
{
    auto &nandFlash{static_cast<NandFlash &>(*nandFlashPtr)};
    return std::to_underlying(nandFlash.writePagesCallback(block, page, mainBuffer, spareBuffer, pages));
}

template <ThreadX::Ulong Blocks, ThreadX::Ulong BlockPages, ThreadX::Ulong PageSize>
auto NandFlash<Blocks, BlockPages, PageSize>::DriverCallback::copyPages(
    ThreadX::Native::LX_NAND_FLASH *nandFlashPtr, ThreadX::Ulong sourceBlock, ThreadX::Ulong sourcePage,
    ThreadX::Ulong destinationBlock, ThreadX::Ulong destinationPage, ThreadX::Ulong pages, ThreadX::Uchar *dataBuffer)
{
    auto &nandFlash{static_cast<NandFlash &>(*nandFlashPtr)};
    return std::to_underlying(
        nandFlash.copyPagesCallback(sourceBlock, sourcePage, destinationBlock, destinationPage, pages, dataBuffer));
}

template <ThreadX::Ulong Blocks, ThreadX::Ulong BlockPages, ThreadX::Ulong PageSize>
auto NandFlash<Blocks, BlockPages, PageSize>::DriverCallback::eraseBlock(
    ThreadX::Native::LX_NAND_FLASH *nandFlashPtr, ThreadX::Ulong block, ThreadX::Ulong eraseCount)
{
    auto &nandFlash{static_cast<NandFlash &>(*nandFlashPtr)};
    return std::to_underlying(nandFlash.eraseBlockCallback(block, eraseCount));
}

template <ThreadX::Ulong Blocks, ThreadX::Ulong BlockPages, ThreadX::Ulong PageSize>
auto NandFlash<Blocks, BlockPages, PageSize>::DriverCallback::verifyErasedBlock(
    ThreadX::Native::LX_NAND_FLASH *nandFlashPtr, ThreadX::Ulong block)
{
    auto &nandFlash{static_cast<NandFlash &>(*nandFlashPtr)};
    return std::to_underlying(nandFlash.verifyErasedBlockCallback(block));
}

template <ThreadX::Ulong Blocks, ThreadX::Ulong BlockPages, ThreadX::Ulong PageSize>
auto NandFlash<Blocks, BlockPages, PageSize>::DriverCallback::verifyErasedPage(
    ThreadX::Native::LX_NAND_FLASH *nandFlashPtr, ThreadX::Ulong block, ThreadX::Ulong page)
{
    auto &nandFlash{static_cast<NandFlash &>(*nandFlashPtr)};
    return std::to_underlying(nandFlash.verifyErasedPageCallback(block, page));
}

template <ThreadX::Ulong Blocks, ThreadX::Ulong BlockPages, ThreadX::Ulong PageSize>
auto NandFlash<Blocks, BlockPages, PageSize>::DriverCallback::getBlockStatus(
    ThreadX::Native::LX_NAND_FLASH *nandFlashPtr, ThreadX::Ulong block, ThreadX::Uchar *badBlockFlag)
{
    auto &nandFlash{static_cast<NandFlash &>(*nandFlashPtr)};
    return std::to_underlying(nandFlash.getBlockStatusCallback(block, badBlockFlag));
}

template <ThreadX::Ulong Blocks, ThreadX::Ulong BlockPages, ThreadX::Ulong PageSize>
auto NandFlash<Blocks, BlockPages, PageSize>::DriverCallback::setBlockStatus(
    ThreadX::Native::LX_NAND_FLASH *nandFlashPtr, ThreadX::Ulong block, ThreadX::Uchar badBlockFlag)
{
    auto &nandFlash{static_cast<NandFlash &>(*nandFlashPtr)};
    return std::to_underlying(nandFlash.setBlockStatusCallback(block, badBlockFlag));
}

template <ThreadX::Ulong Blocks, ThreadX::Ulong BlockPages, ThreadX::Ulong PageSize>
auto NandFlash<Blocks, BlockPages, PageSize>::DriverCallback::getExtraBytes(
    ThreadX::Native::LX_NAND_FLASH *nandFlashPtr, ThreadX::Ulong block, ThreadX::Ulong page,
    ThreadX::Uchar *destination, ThreadX::Uint size)
{
    auto &nandFlash{static_cast<NandFlash &>(*nandFlashPtr)};
    return std::to_underlying(nandFlash.getExtraBytesCallback(block, page, destination, size));
}

template <ThreadX::Ulong Blocks, ThreadX::Ulong BlockPages, ThreadX::Ulong PageSize>
auto NandFlash<Blocks, BlockPages, PageSize>::DriverCallback::setExtraBytes(
    ThreadX::Native::LX_NAND_FLASH *nandFlashPtr, ThreadX::Ulong block, ThreadX::Ulong page, ThreadX::Uchar *source,
    ThreadX::Uint size)
{
    auto &nandFlash{static_cast<NandFlash &>(*nandFlashPtr)};
    return std::to_underlying(nandFlash.setExtraBytesCallback(block, page, source, size));
}

template <ThreadX::Ulong Blocks, ThreadX::Ulong BlockPages, ThreadX::Ulong PageSize>
auto NandFlash<Blocks, BlockPages, PageSize>::DriverCallback::systemError(
    ThreadX::Native::LX_NAND_FLASH *nandFlashPtr, ThreadX::Uint errorCode, ThreadX::Ulong block, ThreadX::Ulong page)
{
    auto &nandFlash{static_cast<NandFlash &>(*nandFlashPtr)};
    return std::to_underlying(nandFlash.systemErrorCallback(errorCode, block, page));
}
} // namespace LevelX
#endif
