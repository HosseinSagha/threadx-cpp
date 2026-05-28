#include "simulatorMediaDriver.hpp"
#include "test.hpp"

std::byte ramMem[10 * 512];

/* There are several useful/important pieces of information contained in
       the media structure, some of which are supplied by FileX and others
       are for the driver to setup. The following is a summary of the
       necessary FX_MEDIA structure members:

            FX_MEDIA Member                    Meaning

        fx_media_driver_request             FileX request type. Valid requests from
                                            FileX are as follows:

                                                    FX_DRIVER_READ
                                                    FX_DRIVER_WRITE
                                                    FX_DRIVER_FLUSH
                                                    FX_DRIVER_ABORT
                                                    FX_DRIVER_INIT
                                                    FX_DRIVER_BOOT_READ
                                                    FX_DRIVER_RELEASE_SECTORS
                                                    FX_DRIVER_BOOT_WRITE
                                                    FX_DRIVER_UNINIT

        fx_media_driver_status              This value is RETURNED by the driver.
                                            If the operation is successful, this
                                            field should be set to FX_SUCCESS for
                                            before returning. Otherwise, if an
                                            error occurred, this field should be
                                            set to FX_IO_ERROR.

        fx_media_driver_buffer              Pointer to buffer to read or write
                                            sector data. This is supplied by
                                            FileX.

        fx_media_driver_logical_sector      Logical sector FileX is requesting.

        fx_media_driver_sectors             Number of sectors FileX is requesting.


       The following is a summary of the optional FX_MEDIA structure members:

            FX_MEDIA Member                              Meaning

        fx_media_driver_info                Pointer to any additional information
                                            or memory. This is optional for the
                                            driver use and is setup from the
                                            fx_media_open call. The RAM disk uses
                                            this pointer for the RAM disk memory
                                            itself.

        fx_media_driver_write_protect       The DRIVER sets this to FX_TRUE when
                                            media is write protected. This is
                                            typically done in initialization,
                                            but can be done anytime.

        fx_media_driver_free_sector_update  The DRIVER sets this to FX_TRUE when
                                            it needs to know when clusters are
                                            released. This is important for FLASH
                                            wear-leveling drivers.

        fx_media_driver_system_write        FileX sets this flag to FX_TRUE if the
                                            sector being written is a system sector,
                                            e.g., a boot, FAT, or directory sector.
                                            The driver may choose to use this to
                                            initiate error recovery logic for greater
                                            fault tolerance.

        fx_media_driver_data_sector_read    FileX sets this flag to FX_TRUE if the
                                            sector(s) being read are file data sectors,
                                            i.e., NOT system sectors.

        fx_media_driver_sector_type         FileX sets this variable to the specific
                                            type of sector being read or written. The
                                            following sector types are identified:

                                                    FX_UNKNOWN_SECTOR
                                                    FX_BOOT_SECTOR
                                                    FX_FAT_SECTOR
                                                    FX_DIRECTORY_SECTOR
                                                    FX_DATA_SECTOR
     */

void ramMediaDriver(FileX::Media<>::InternalDriver &ramMedia)
{
    ThreadX::Uchar *source_buffer;
    ThreadX::Uchar *destination_buffer;
    ThreadX::Uint bytes_per_sector;

    ramMedia.status(FileX::Error::success);

    /* Process the driver request specified in the media control block.  */
    switch (ramMedia.request())
    {
    case FileX::MediaDriverRequest::read:
        /* Calculate the RAM disk sector offset. Note the RAM disk memory is pointed to by
           the fx_media_driver_info pointer, which is supplied by the application in the
           call to fx_media_open.  */
        source_buffer = static_cast<ThreadX::Uchar *>(ramMedia.info()) +
                        (ramMedia.logicalSector() * std::to_underlying(ramMedia.sectorSize()));

        /* Copy the RAM sector into the destination.  */
        ThreadX::Native::_fx_utility_memory_copy(
            source_buffer, ramMedia.buffer(), ramMedia.sectors() * std::to_underlying(ramMedia.sectorSize()));

        return;

    case FileX::MediaDriverRequest::write:
        /* Calculate the RAM disk sector offset. Note the RAM disk memory is pointed to by
           the fx_media_driver_info pointer, which is supplied by the application in the
           call to fx_media_open.  */
        destination_buffer = static_cast<ThreadX::Uchar *>(ramMedia.info()) +
                             (ramMedia.logicalSector() * std::to_underlying(ramMedia.sectorSize()));

        /* Copy the source to the RAM sector.  */
        ThreadX::Native::_fx_utility_memory_copy(
            ramMedia.buffer(), destination_buffer, ramMedia.sectors() * std::to_underlying(ramMedia.sectorSize()));

        return;

    case FileX::MediaDriverRequest::bootRead:
        /* Read the boot record and return to the caller.  */

        /* Calculate the RAM disk boot sector offset, which is at the very beginning of
           the RAM disk. Note the RAM disk memory is pointed to by the
           fx_media_driver_info pointer, which is supplied by the application in the
           call to fx_media_open.  */
        source_buffer = static_cast<ThreadX::Uchar *>(ramMedia.info());

        /* For RAM driver, determine if the boot record is valid.  */
        if ((source_buffer[0] != 0xEB) or ((source_buffer[1] != 0x34) and (source_buffer[1] != 0x76)) or
            (source_buffer[2] != 0x90))
        {
            /* Invalid boot record, return an error!  */
            ramMedia.status(FileX::Error::mediaInvalid);
            return;
        }

        /* For RAM disk only, pickup the bytes per sector.  */
        bytes_per_sector = ThreadX::Native::_fx_utility_16_unsigned_read(&source_buffer[FX_BYTES_SECTOR]);

        /* Ensure this is less than the media memory size.  */
        if (bytes_per_sector > std::to_underlying(ramMedia.sectorSize())) // fx_media_memory_size
        {
            ramMedia.status(FileX::Error::bufferError);
            break;
        }

        /* Copy the RAM boot sector into the destination.  */
        ThreadX::Native::_fx_utility_memory_copy(source_buffer, ramMedia.buffer(), bytes_per_sector);

        return;

    case FileX::MediaDriverRequest::bootWrite:
        /* Write the boot record and return to the caller.  */

        /* Calculate the RAM disk boot sector offset, which is at the very beginning of the
           RAM disk. Note the RAM disk memory is pointed to by the fx_media_driver_info
           pointer, which is supplied by the application in the call to fx_media_open.  */
        destination_buffer = static_cast<ThreadX::Uchar *>(ramMedia.info());

        /* Copy the RAM boot sector into the destination.  */
        ThreadX::Native::_fx_utility_memory_copy(
            ramMedia.buffer(), destination_buffer, std::to_underlying(ramMedia.sectorSize()));

        return;

    case FileX::MediaDriverRequest::flush:
    case FileX::MediaDriverRequest::abort:
    case FileX::MediaDriverRequest::init:
    case FileX::MediaDriverRequest::uninit: /* There is nothing to do in this case for the RAM driver.  For actual
           devices some shutdown processing may be necessary.  */

        /* FLASH drivers are responsible for setting several fields in the
           media structure, as follows:

                media_ptr -> fx_media_driver_free_sector_update
                media_ptr -> fx_media_driver_write_protect

           The fx_media_driver_free_sector_update flag is used to instruct
           FileX to inform the driver whenever sectors are not being used.
           This is especially useful for FLASH managers so they don't have
           maintain mapping for sectors no longer in use.

           The fx_media_driver_write_protect flag can be set anytime by the
           driver to indicate the media is not writable.  Write attempts made
           when this flag is set are returned as errors.  */

        /* Perform basic initialization here... since the boot record is going
           to be read subsequently and again for volume name requests.  */
        return;

    case FileX::MediaDriverRequest::releaseSectors:
    default:
        /* Invalid driver request.  */
        ramMedia.status(FileX::Error::ioError);
        return;
    }
}

void norFlashSimulatorMediaDriver(FileX::Media<NorFlash::sectorSize()>::InternalDriver &norMedia, NorFlash &norFlash)
{
    ThreadX::Uchar *source_buffer;
    ThreadX::Uchar *destination_buffer;
    ThreadX::Ulong logical_sector;

    norMedia.status(FileX::Error::success);

    /* Process the driver request specified in the media control block.  */
    switch (norMedia.request())
    {
    case FileX::MediaDriverRequest::read:

        /* Setup the destination buffer and logical sector.  */
        logical_sector = norMedia.logicalSector();
        destination_buffer = norMedia.buffer();

        /* Loop to read sectors from flash.  */
        for (ThreadX::Ulong i{}; i < norMedia.sectors(); i++)
        {
/* Read a sector from NOR flash.  */
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wcast-align"
            if (norFlash.readSector(
                    logical_sector,
                    std::span<ThreadX::Ulong, std::to_underlying(norMedia.sectorSize()) / ThreadX::wordSize>(
                        reinterpret_cast<ThreadX::Ulong *>(destination_buffer),
                        std::to_underlying(norMedia.sectorSize()) / ThreadX::wordSize)) != LevelX::Error::success)
#pragma GCC diagnostic pop
            {
                norMedia.status(FileX::Error::ioError);
                return;
            }

            /* Move to the next entries.  */
            ++logical_sector;
            destination_buffer += std::to_underlying(norMedia.sectorSize());
        }

        return;

    case FileX::MediaDriverRequest::write:
        /* Setup the source buffer and logical sector.  */
        logical_sector = norMedia.logicalSector();
        source_buffer = norMedia.buffer();

        /* Loop to write sectors to flash.  */
        for (ThreadX::Ulong i{}; i < norMedia.sectors(); i++)
        {
/* Write a sector to NOR flash.  */
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wcast-align" // driverBuffer() is always word aligned by media class design
            if (norFlash.writeSector(
                    logical_sector,
                    std::span<ThreadX::Ulong, std::to_underlying(norMedia.sectorSize()) / ThreadX::wordSize>(
                        reinterpret_cast<ThreadX::Ulong *>(source_buffer),
                        std::to_underlying(norMedia.sectorSize()) / ThreadX::wordSize)) != LevelX::Error::success)
#pragma GCC diagnostic pop
            {
                norMedia.status(FileX::Error::ioError);
                return;
            }

            /* Move to the next entries.  */
            logical_sector++;
            source_buffer = source_buffer + std::to_underlying(norMedia.sectorSize());
        }

        return;

    case FileX::MediaDriverRequest::releaseSectors:

        /* Setup the logical sector.  */
        logical_sector = norMedia.logicalSector();

        /* Release sectors.  */
        for (ThreadX::Ulong i{}; i < norMedia.sectors(); i++)
        {
            /* Release NOR flash sector.  */
            if (norFlash.releaseSector(logical_sector) != LevelX::Error::success)
            {
                norMedia.status(FileX::Error::ioError);
                return;
            }

            /* Move to the next entries.  */
            logical_sector++;
        }

        return;

    case FileX::MediaDriverRequest::init:
        /* Perform basic initialization here... since the boot record is going
               to be read subsequently and again for volume name requests.  */

        /* With flash wear leveling, FileX should tell wear leveling when sectors
               are no longer in use.  */
        norMedia.freeSectorUpdate();

        /* Open the NOR flash simulation.  */
        if (norFlash.open() != LevelX::Error::success)
        {
            norMedia.status(FileX::Error::ioError);
        }

        return;

    case FileX::MediaDriverRequest::uninit:
        /* There is nothing to do in this case for the RAM driver.  For actual
               devices some shutdown processing may be necessary.  */

        /* Close the NOR flash simulation.  */
        if (norFlash.close() != LevelX::Error::success)
        {
            norMedia.status(FileX::Error::ioError);
        }

        return;

    case FileX::MediaDriverRequest::bootRead:
        /* Read the boot record and return to the caller.  */

        /* Setup the destination buffer.  */
        destination_buffer = norMedia.buffer();

/* Read boot sector from NOR flash.  */
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wcast-align"
        if (norFlash.readSector(
                LevelX::norBootSector,
                std::span<ThreadX::Ulong, std::to_underlying(norMedia.sectorSize()) / ThreadX::wordSize>(
                    reinterpret_cast<ThreadX::Ulong *>(destination_buffer),
                    std::to_underlying(norMedia.sectorSize()) / ThreadX::wordSize)) != LevelX::Error::success)
#pragma GCC diagnostic pop
        {
            norMedia.status(FileX::Error::ioError);
            return;
        }

        /* For NOR driver, determine if the boot record is valid.  */
        if (destination_buffer[0] != 0xEB or destination_buffer[1] != 0x34 or destination_buffer[2] != 0x90)
        {
            /* Invalid boot record, return an error!  */
            norMedia.status(FileX::Error::mediaInvalid);
        }

        return;

    case FileX::MediaDriverRequest::bootWrite:
        /* Make sure the media bytes per sector equals to the LevelX logical sector size.  */
        if (norMedia.sectorSize() != norFlash.sectorSize())
        {
            /* Sector size mismatch, return error.  */
            norMedia.status(FileX::Error::ioError);
            return;
        }

        /* Write the boot record and return to the caller.  */

        /* Setup the source buffer.  */
        source_buffer = norMedia.buffer();

/* Write boot sector to NOR flash.  */
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wcast-align"
        if (norFlash.writeSector(
                LevelX::norBootSector,
                std::span<ThreadX::Ulong, std::to_underlying(norMedia.sectorSize()) / ThreadX::wordSize>{
                    reinterpret_cast<ThreadX::Ulong *>(source_buffer), LevelX::norSectorSizeInWord}) !=
            LevelX::Error::success)
#pragma GCC diagnostic pop
        {
            norMedia.status(FileX::Error::ioError);
        }

        return;

    case FileX::MediaDriverRequest::flush:
    case FileX::MediaDriverRequest::abort:
        return;

    default:
        /* Invalid driver request.  */
        norMedia.status(FileX::Error::ioError);
        return;
    }
}

#if 0
void nandFlashSimulatorMediaDriver(NandMedia &nandMedia)
{
    ThreadX::Ulong logical_sector;
    ThreadX::Ulong count;
    ThreadX::Uchar *buffer;

    nandMedia.driverStatus(FileX::Error::success);

    /* Process the driver request specified in the media control block.  */
    switch (nandMedia.driverRequest())
    {
    case FileX::MediaDriverRequest::read:
        /* Read sector(s) from NAND flash.  */
        logical_sector = nandMedia.driverLogicalSector();
        count = nandMedia.driverSectors();
        buffer = nandMedia.driverBuffer();

/* Call LevelX to read one flash sector.  */
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wcast-align"
        if (nandMedia.m_nandFlash.readSectors(
                logical_sector, {reinterpret_cast<ThreadX::Ulong *>(buffer),
                                 count * std::to_underlying(nandMedia.sectorSize()) / ThreadX::wordSize}) !=
            LevelX::Error::success)
#pragma GCC diagnostic pop
        {
            /* Return an I/O error to FileX.  */
            nandMedia.driverStatus(FileX::Error::ioError);
            return;
        }

        return;

    case FileX::MediaDriverRequest::write:
        /* Write sector(s) to NAND flash.  */
        logical_sector = nandMedia.driverLogicalSector();
        count = nandMedia.driverSectors();
        buffer = nandMedia.driverBuffer();

        /* Call LevelX to write a sector.  */
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wcast-align"
        if (nandMedia.m_nandFlash.writeSectors(
                logical_sector, {reinterpret_cast<ThreadX::Ulong *>(buffer),
                                 count * std::to_underlying(nandMedia.sectorSize()) / ThreadX::wordSize}) !=
            LevelX::Error::success)
#pragma GCC diagnostic pop
        {
            /* Return an I/O error to FileX.  */
            nandMedia.driverStatus(FileX::Error::ioError);
            return;
        }

        return;

    case FileX::MediaDriverRequest::releaseSectors:
        /* Release the mapping of this sector.  */
        logical_sector = nandMedia.driverLogicalSector();
        count = nandMedia.driverSectors();
        while (count)
        {
            /* Call LevelX to release a sector mapping.  */
            if (nandMedia.m_nandFlash.releaseSectors(logical_sector) != LevelX::Error::success)
            {
                /* Return an I/O error to FileX.  */
                nandMedia.driverStatus(FileX::Error::ioError);
                return;
            }

            /* Successful sector release.  */
            count--;
            logical_sector++;
        }

        return;

    case FileX::MediaDriverRequest::init:
        /* With flash wear leveling, FileX should tell wear leveling when sectors
               are no longer in use.  */
        nandMedia.driverFreeSectorUpdate();

        /* Open the NAND flash simulation.  */
        if (nandMedia.m_nandFlash.open() != LevelX::Error::success)
        {
            /* Return an I/O error to FileX.  */
            nandMedia.driverStatus(FileX::Error::ioError);
            return;
        }

        return;

    case FileX::MediaDriverRequest::uninit:
        /* There is nothing to do in this case for the RAM driver.  For actual
               devices some shutdown processing may be necessary.  */

        /* Close the NAND flash simulation.  */
        if (nandMedia.m_nandFlash.close() != LevelX::Error::success)
        {
            /* Return an I/O error to FileX.  */
            nandMedia.driverStatus(FileX::Error::ioError);
            return;
        }

        return;

    case FileX::MediaDriverRequest::bootRead:
/* Read the boot record and return to the caller.  */
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wcast-align"
        if (nandMedia.m_nandFlash.readSectors(
                LevelX::nandBootSector,
                {reinterpret_cast<ThreadX::Ulong *>(nandMedia.driverBuffer()),
                 std::to_underlying(nandMedia.sectorSize()) / ThreadX::wordSize}) != LevelX::Error::success)
#pragma GCC diagnostic pop
        {
            /* Return an I/O error to FileX.  */
            nandMedia.driverStatus(FileX::Error::ioError);
            return;
        }

        return;

    case FileX::MediaDriverRequest::bootWrite:
/* Write the boot record and return to the caller.  */
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wcast-align"
        if (nandMedia.m_nandFlash.writeSectors(
                LevelX::nandBootSector,
                {reinterpret_cast<ThreadX::Ulong *>(nandMedia.driverBuffer()),
                 std::to_underlying(nandMedia.sectorSize()) / ThreadX::wordSize}) != LevelX::Error::success)
#pragma GCC diagnostic pop
        {
            /* Return an I/O error to FileX.  */
            nandMedia.driverStatus(FileX::Error::ioError);
            return;
        }

        return;

    case FileX::MediaDriverRequest::flush:
    case FileX::MediaDriverRequest::abort:
        return;

    default:
        /* Invalid driver request.  */
        nandMedia.driverStatus(FileX::Error::ioError);
        return;
    }
}
#endif
