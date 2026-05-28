#include "flashSimulator.hpp"
#include "test.hpp"

#if 0
#define BAD_BLOCK_POSITION 0  /*      0 is the bad block byte postion                         */
#define EXTRA_BYTE_POSITION 0 /*      0 is the extra bytes starting byte postion              */
#define ECC_BYTE_POSITION 8   /*      8 is the ECC starting byte position                     */
#endif

alignas(ThreadX::Ulong) NorFlash::Block norMem[norBlocks];
#if 0
alignas(ThreadX::Ulong) NandFlash::Block nandMem[nandBlocks];
alignas(ThreadX::Ulong) NandFlash::BlockDiag nandBlockDiag[nandBlocks];
#endif

[[gnu::const]] LevelX::Error NorFlashSimulatorDriver::initialise()
{
    return LevelX::Error::success;
}

LevelX::Error NorFlashSimulatorDriver::read(const ThreadX::Ulong *flashAddress,
                                            const std::span<ThreadX::Ulong> destination)
{
    /* Loop to read flash.  */
    auto remainingWords{destination.size()};
    auto destinationPtr{destination.data()};
    while (remainingWords--)
    {
        /* Copy word.  */
        *destinationPtr++ = *flashAddress++;
    }

    return LevelX::Error::success;
}

LevelX::Error NorFlashSimulatorDriver::write(ThreadX::Ulong *flashAddress, const std::span<const ThreadX::Ulong> source)
{
    /* Loop to write flash.  */
    auto remainingWords{source.size()};
    auto sourcePtr{source.data()};
    while (remainingWords--)
    {
        /* Copy word.  */
        *flashAddress++ = *sourcePtr++;
    }

    return LevelX::Error::success;
}

LevelX::Error NorFlashSimulatorDriver::eraseBlock(const ThreadX::Ulong block,
                                                  [[maybe_unused]] const ThreadX::Ulong blockCount)
{
    ThreadX::Ulong *pointer;
    ThreadX::Ulong words;

    /* Setup pointer.  */
    pointer = reinterpret_cast<ThreadX::Ulong *>(&norMem[block]);

    /* Loop to erase block.  */
    words = sizeof(NorFlash::Block) / sizeof(ThreadX::Ulong);
    while (words--)
    {
        /* Erase word of block.  */
        *pointer++ = 0xFFFFFFFFUL;
    }

    return LevelX::Error::success;
}

[[gnu::pure]] LevelX::Error NorFlashSimulatorDriver::verifyErasedBlock(const ThreadX::Ulong block)
{
    ThreadX::Ulong *word_ptr;
    ThreadX::Ulong words;

    /* Determine if the block is completely erased.  */

    /* Pickup the pointer to the first word of the block.  */
    word_ptr = &norMem[block].eraseCount;

    /* Calculate the number of words in a block.  */
    words = sizeof(NorFlash::Block) / sizeof(ThreadX::Ulong);

    /* Loop to check if the block is erased.  */
    while (words--)
    {
        /* Is this word erased?  */
        if (*word_ptr++ != 0xFFFFFFFF)
            return LevelX::Error::error;
    }

    /* Return success.  */
    return LevelX::Error::success;
}

[[gnu::const]] LevelX::Error NorFlashSimulatorDriver::systemError([[maybe_unused]] const ThreadX::Uint errorCode)
{
    return LevelX::Error::success;
}

LevelX::Error norFlashSimulatorEraseAll()
{
    ThreadX::Ulong *pointer;
    ThreadX::Ulong words;

    /* Setup pointer.  */
    pointer = reinterpret_cast<ThreadX::Ulong *>(&norMem[0]);

    /* Loop to erase block.  */
    words = sizeof(norMem) / (sizeof(ThreadX::Ulong));
    while (words--)
    {
        /* Erase word of block.  */
        *pointer++ = 0xFFFFFFFFUL;
    }

    return LevelX::Error::success;
}

#if 0
LevelX::Error nandFlashSimulatorRead(
    NandFlash *nandFlash, ThreadX::Ulong block, ThreadX::Ulong page, ThreadX::Ulong *destination, ThreadX::Ulong words)
{
    auto status{nandFlash->checkPageEcc(
        std::span<ThreadX::Uchar, NandFlash::pageDataAndSpareSizes().first / ThreadX::wordSize>(
            reinterpret_cast<ThreadX::Uchar *>(nandMem[block].physicalPages[page].memory),
            NandFlash::pageDataAndSpareSizes().first / ThreadX::wordSize),
        std::span<ThreadX::Uchar, NandFlash::eccSize>(
            &nandMem[block].physicalPages[page].spare[ECC_BYTE_POSITION], NandFlash::eccSize))};

    if (status == LevelX::Error::nandErrorNotCorrected)
    {
        status = LevelX::Error::error;
    }

    /* Pickup the flash address.  */
    ThreadX::Ulong *flash_address = &(nandMem[block].physicalPages[page].memory[0]);

    /* Loop to read flash.  */
    while (words--)
    {
        /* Copy word.  */
        *destination++ = *flash_address++;
    }

    return status;
}

LevelX::Error nandFlashSimulatorPagesRead(
    NandFlash *nandFlash, ThreadX::Ulong block, ThreadX::Ulong page, ThreadX::Uchar *main_buffer,
    ThreadX::Uchar *spare_buffer, ThreadX::Ulong pages)
{
    ThreadX::Uint i;
    LevelX::Error status;
    LevelX::Error ecc_status = LevelX::Error::success;

    for (i = 0; i < pages; i++)
    {
        if (main_buffer)
        {
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wcast-align"
            status = nandFlashSimulatorRead(
                nandFlash, block, page + i,
                reinterpret_cast<ThreadX::Ulong *>(main_buffer + i * nandFlash->pageDataSize()),
                nandFlash->pageDataSize() / ThreadX::wordSize);
#pragma GCC diagnostic pop

            if (status == LevelX::Error::nandErrorCorrected)
            {
                ecc_status = LevelX::Error::nandErrorCorrected;
            }
            else if (status == LevelX::Error::nandErrorNotCorrected)
            {
                ecc_status = LevelX::Error::error;
                break;
            }
        }

        status = nandFlashSimulatorExtraBytesGet(
            block, page + i, spare_buffer + i * NandFlash::spareBytesSize(), NandFlash::spareBytesSize());
    }

    return (ecc_status);
}

LevelX::Error nandFlashSimulatorWrite(
    NandFlash *nandFlash, ThreadX::Ulong block, ThreadX::Ulong page, ThreadX::Ulong *source, ThreadX::Ulong words)
{
    ThreadX::Ulong *flash_address;
    ThreadX::Uchar *flash_spare_address;
    ThreadX::Uint ecc_bytes = 6;
    ThreadX::Uchar new_ecc_buffer[6];
    ThreadX::Uchar *ecc_buffer_ptr = new_ecc_buffer;

    /* Increment the diag info.  */
    nandBlockDiag[block].pageWrites[page]++;
    if (nandBlockDiag[block].pageWrites[page] > nandBlockDiag[block].maxPageWrites[page])
        nandBlockDiag[block].maxPageWrites[page] = nandBlockDiag[block].pageWrites[page];

    /* Pickup the flash address.  */
    flash_address = &(nandMem[block].physicalPages[page].memory[0]);

    /* Loop to write flash.  */
    while (words--)
    {
        /* Can the word be written?  We can clear new bits, but just can't unclear
           in a NAND device.  */
        if ((*source & *flash_address) != *source)
        {
            return LevelX::Error::invalidWrite;
        }

        /* Copy word.  */
        *flash_address++ = *source++;
    }

    /* Compute the ECC for this 256 byte piece of the page.  */
    nandFlash->computePageEcc(
        std::span<ThreadX::Uchar, NandFlash::pageDataAndSpareSizes().first / ThreadX::wordSize>(
            reinterpret_cast<ThreadX::Uchar *>(nandMem[block].physicalPages[page].memory),
            NandFlash::pageDataAndSpareSizes().first / ThreadX::wordSize),
        new_ecc_buffer);

    /* Setup destination pointer in the spare area.  */
    flash_spare_address = &nandMem[block].physicalPages[page].spare[ECC_BYTE_POSITION];
    while (ecc_bytes--)
    {

        /* Can the word be written?  We can clear new bits, but just can't unclear
           in a NAND device.  */
        if ((*ecc_buffer_ptr & *flash_spare_address) != *ecc_buffer_ptr)
        {
            return LevelX::Error::invalidWrite;
        }

        /* Set an ecc byte in the spare area.  */
        *flash_spare_address++ = *ecc_buffer_ptr++;
    }

    return LevelX::Error::success;
}

LevelX::Error nandFlashSimulatorPagesWrite(
    NandFlash *nandFlash, ThreadX::Ulong block, ThreadX::Ulong page, ThreadX::Uchar *main_buffer,
    ThreadX::Uchar *spare_buffer, ThreadX::Ulong pages)
{
    for (ThreadX::Ulong i{}; i < pages; i++)
    {
        nandFlashSimulatorExtraBytesSet(
            block, page + i, spare_buffer + i * NandFlash::spareBytesSize(), NandFlash::spareBytesSize());
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wcast-align"
        if (auto status{nandFlashSimulatorWrite(
                nandFlash, block, page + i,
                reinterpret_cast<ThreadX::Ulong *>(main_buffer + i * nandFlash->pageDataSize()),
                nandFlash->pageDataSize() / ThreadX::wordSize)};
            status == LevelX::Error::invalidWrite)
#pragma GCC diagnostic pop
        {
            return status;
        }
    }

    return LevelX::Error::success;
}

LevelX::Error nandFlashSimulatorPagesCopy(
    NandFlash *nandFlash, ThreadX::Ulong source_block, ThreadX::Ulong source_page, ThreadX::Ulong destination_block,
    ThreadX::Ulong destination_page, ThreadX::Ulong pages, ThreadX::Uchar *data_buffer)
{
    for (ThreadX::Ulong i{}; i < pages; i++)
    {
        if (auto err{nandFlashSimulatorPagesRead(
                nandFlash, source_block, source_page + i, data_buffer, data_buffer + nandFlash->pageDataSize(), 1)};
            err != LevelX::Error::success and err != LevelX::Error::nandErrorCorrected)
        {
            return err;
        }

        if (auto err{nandFlashSimulatorPagesWrite(nandFlash, destination_block, destination_page + i, data_buffer,
                                                  data_buffer + nandFlash->pageDataSize(), 1)};
            err != LevelX::Error::success)
        {
            return err;
        }
    }

    return LevelX::Error::success;
}

LevelX::Error nandFlashSimulatorBlockErase(ThreadX::Ulong block, [[maybe_unused]] ThreadX::Ulong erase_count)
{
    ThreadX::Ulong *pointer;
    ThreadX::Ulong words;
    ThreadX::Uint i;

    /* Increment the diag info.  */
    nandBlockDiag[block].erases++;
    for (i = 0; i < NandFlash::blockPages(); i++)
        nandBlockDiag[block].pageWrites[i] = 0;

    /* Setup pointer.  */
    pointer = reinterpret_cast<ThreadX::Ulong *>(&nandMem[block]);

    /* Loop to erase block.  */
    words = sizeof(NandFlash::Block) / sizeof(ThreadX::Ulong);
    while (words--)
    {

        /* Erase word of block.  */
        *pointer++ = 0xFFFFFFFFUL;
    }

    return LevelX::Error::success;
}

LevelX::Error nandFlashSimulatorEraseAll()
{
    ThreadX::Ulong *pointer;
    ThreadX::Ulong words;
    ThreadX::Uint i, j;

    /* Increment the diag info.  */
    for (i = 0; i < nandBlocks; i++)
    {
        nandBlockDiag[i].erases = 0;
        for (j = 0; j < NandFlash::blockPages(); j++)
            nandBlockDiag[i].pageWrites[j] = 0;
    }

    /* Setup pointer.  */
    pointer = reinterpret_cast<ThreadX::Ulong *>(&nandMem[0]);

    /* Loop to erase block.  */
    words = sizeof(nandMem) / (sizeof(ThreadX::Ulong));
    while (words--)
    {

        /* Erase word of block.  */
        *pointer++ = 0xFFFFFFFFUL;
    }

    return LevelX::Error::success;
}

LevelX::Error nandFlashSimulatorBlockErasedVerify(ThreadX::Ulong block)
{
    ThreadX::Ulong *word_ptr;
    ThreadX::Ulong words;

    /* Determine if the block is completely erased.  */

    /* Pickup the pointer to the first word of the block.  */
    word_ptr = reinterpret_cast<ThreadX::Ulong *>(&nandMem[block]);

    /* Calculate the number of words in a block.  */
    words = sizeof(NandFlash::Block) / ThreadX::wordSize;

    /* Loop to check if the block is erased.  */
    while (words--)
    {
        /* Is this word erased?  */
        if (*word_ptr++ != 0xFFFFFFFF)
        {
            return LevelX::Error::error;
        }
    }

    return LevelX::Error::success;
}

LevelX::Error nandFlashSimulatorPageErasedVerify(ThreadX::Ulong block, ThreadX::Ulong page)
{
    ThreadX::Ulong *word_ptr;
    ThreadX::Ulong words;

    /* Determine if the block is completely erased.  */

    /* Pickup the pointer to the first word of the block's page.  */
    word_ptr = reinterpret_cast<ThreadX::Ulong *>(nandMem[block].physicalPages[page].memory);

    /* Calculate the number of words in a block.  */
    words = NandFlash::pageDataSize() / ThreadX::wordSize;

    /* Loop to check if the page is erased.  */
    while (words--)
    {
        /* Is this word erased?  */
        if (*word_ptr++ != 0xFFFFFFFF)
        {
            return LevelX::Error::error;
        }
    }

    return LevelX::Error::success;
}

LevelX::Error nandFlashSimulatorBlockStatusGet(ThreadX::Ulong block, ThreadX::Uchar *bad_block_byte)
{
    /* Pickup the bad block byte and return it.  */
    *bad_block_byte = nandMem[block].physicalPages[0].spare[BAD_BLOCK_POSITION];

    return LevelX::Error::success;
}

LevelX::Error nandFlashSimulatorBlockStatusSet(ThreadX::Ulong block, ThreadX::Uchar bad_block_byte)
{
    /* Set the bad block byte.  */
    nandMem[block].physicalPages[0].spare[BAD_BLOCK_POSITION] = bad_block_byte;

    return LevelX::Error::success;
}

LevelX::Error nandFlashSimulatorExtraBytesGet(
    ThreadX::Ulong block, ThreadX::Ulong page, ThreadX::Uchar *destination, ThreadX::Uint size)
{
    ThreadX::Uchar *source;

    /* Setup source pointer in the spare area.  */
    source = &nandMem[block].physicalPages[page].spare[EXTRA_BYTE_POSITION];

    /* Loop to return the extra bytes requested.  */
    while (size--)
    {

        /* Retrieve an extra byte from the spare area.  */
        *destination++ = *source++;
    }

    return LevelX::Error::success;
}

LevelX::Error nandFlashSimulatorExtraBytesSet(
    ThreadX::Ulong block, ThreadX::Ulong page, ThreadX::Uchar *source, ThreadX::Uint size)
{
    ThreadX::Uchar *destination;

    /* Increment the diag info.  */
    nandBlockDiag[block].pageWrites[page]++;
    if (nandBlockDiag[block].pageWrites[page] > nandBlockDiag[block].maxPageWrites[page])
        nandBlockDiag[block].maxPageWrites[page] = nandBlockDiag[block].pageWrites[page];

    /* Setup destination pointer in the spare area.  */
    destination = &nandMem[block].physicalPages[page].spare[EXTRA_BYTE_POSITION];

    /* Loop to set the extra bytes.  */
    while (size--)
    {

        /* Set an extra byte in the spare area.  */
        *destination++ = *source++;
    }

    return LevelX::Error::success;
}
#endif
