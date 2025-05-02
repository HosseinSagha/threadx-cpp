#pragma once

#include "fxCommon.hpp"
#include "tickTimer.hpp"
#include <array>
#include <atomic>
#include <expected>
#include <functional>
#include <span>
#include <string_view>
#include <utility>

namespace FileX
{
enum class FaultTolerantMode : bool
{
    disable,
    enable
};

enum class MediaSectorType : ThreadX::Uint
{
    unknown = FX_UNKNOWN_SECTOR,
    boot = FX_BOOT_SECTOR,
    fat = FX_FAT_SECTOR,
    dirctory = FX_DIRECTORY_SECTOR,
    data = FX_DATA_SECTOR
};

enum class MediaDriverRequest : ThreadX::Uint
{
    read = FX_DRIVER_READ,
    write = FX_DRIVER_WRITE,
    flush = FX_DRIVER_FLUSH,
    abort = FX_DRIVER_ABORT,
    init = FX_DRIVER_INIT,
    bootRead = FX_DRIVER_BOOT_READ,
    releaseSectors = FX_DRIVER_RELEASE_SECTORS,
    bootWrite = FX_DRIVER_BOOT_WRITE,
    uninit = FX_DRIVER_UNINIT
};

class MediaBase
{
  protected:
    explicit MediaBase() = default;

    static inline std::atomic_flag m_fileSystemInitialised = ATOMIC_FLAG_INIT;
};

template <MediaSectorSize N = defaultSectorSize>
class Media : ThreadX::Native::FX_MEDIA, MediaBase
{
  public:
    friend class File;

    using NotifyCallback = std::function<void(Media &)>;
    using ExpectedUlong64 = std::expected<ThreadX::Ulong64, Error>;
    using ExpectedStr = std::expected<std::string_view, Error>;

    Media(const Media &) = delete;
    Media &operator=(const Media &) = delete;

    static constexpr MediaSectorSize sectorSize();
    static auto setFileSystemTime();
    // Once initialized by this constructor, the application should call fx_system_date_set and fx_system_time_set to start with an accurate system date and
    // time.
    explicit Media(std::byte *driverInfoPtr = nullptr, const NotifyCallback &openNotifyCallback = {}, const NotifyCallback &closeNotifyCallback = {});

    auto open(const std::string_view name, const FaultTolerantMode mode = FaultTolerantMode::enable);
    auto format(const std::string_view volumeName, const ThreadX::Ulong storageSize, const ThreadX::Uint sectorPerCluster = 1,
                const ThreadX::Uint directoryEntriesFat12_16 = 32);
    auto volume(const std::string_view volumeName);
    auto volume() -> ExpectedStr;
    auto createDir(const std::string_view dirName);
    auto deleteDir(const std::string_view dirName);
    auto renameDir(const std::string_view dirName, const std::string_view newName);
    auto createFile(const std::string_view fileName);
    auto deleteFile(const std::string_view fileName);
    auto renameFile(const std::string_view fileName, const std::string_view newFileName);
    auto defaultDir(const std::string_view newPath);
    auto defaultDir() -> ExpectedStr;
    auto localDir(const std::string_view newPath);
    auto localDir() -> ExpectedStr;
    auto clearLocalDir();
    auto space() -> ExpectedUlong64;
    /// This service is typically called when I/O errors are detected
    auto abort();
    auto invalidateCache();
    auto check();
    auto flush();
    auto close();
    auto name() const;
    auto writeSector(const ThreadX::Ulong sectorNo, const std::span<std::byte, std::to_underlying(N)> sectorData);
    auto readSector(const ThreadX::Ulong sectorNo, std::span<std::byte, std::to_underlying(N)> sectorData);

    auto driverInfo();
    auto driverRequest();
    auto driverStatus(const Error error);
    auto driverBuffer();
    auto driverLogicalSector();
    auto driverSectors();
    auto driverPhysicalSector();
    auto driverPhysicalTrack();
    auto driverWriteProtect(const bool writeProtect = true);
    auto driverFreeSectorUpdate(const bool freeSectorUpdate = true);
    auto driverSystemWrite();
    auto driverDataSectorRead();
    auto driverSectorType();

    virtual void driverCallback() = 0;

  protected:
    virtual ~Media();

  private:
    static auto driverCallback(auto mediaPtr);
    static auto openNotifyCallback(auto mediaPtr);
    static auto closeNotifyCallback(auto mediaPtr);

#ifdef FX_ENABLE_FAULT_TOLERANT
    static constexpr ThreadX::Uint m_faultTolerantCacheSize{FX_FAULT_TOLERANT_MAXIMUM_LOG_FILE_SIZE};
    static_assert(m_faultTolerantCacheSize % ThreadX::wordSize == 0, "Fault tolerant cache size must be a multiple of word size.");
    // the scratch memory size shall be at least 3072 bytes and must be multiple of sector size.
    static constexpr auto cacheSize = []() {
        return (std::to_underlying(N) > std::to_underlying(MediaSectorSize::oneKiloByte))
                   ? std::to_underlying(MediaSectorSize::fourKilobytes) / ThreadX::wordSize
                   : m_faultTolerantCacheSize / ThreadX::wordSize;
    };
    std::array<ThreadX::Ulong, cacheSize()> m_faultTolerantCache{};
#endif
    std::byte *m_driverInfoPtr;
    const NotifyCallback m_openNotifyCallback;
    const NotifyCallback m_closeNotifyCallback;
    static constexpr size_t volumNameLength{12};
    std::array<ThreadX::Ulong, std::to_underlying(N) / ThreadX::wordSize> m_mediaMemory{};
    const ThreadX::Ulong m_mediaMemorySizeInBytes{m_mediaMemory.size() * ThreadX::wordSize};
};

template <MediaSectorSize N>
constexpr MediaSectorSize Media<N>::sectorSize()
{
    return N;
}

template <MediaSectorSize N>
auto Media<N>::setFileSystemTime()
{
    auto time{std::chrono::system_clock::to_time_t(std::chrono::system_clock::now())};
    auto localTime{*std::localtime(std::addressof(time))};

    if (Error error{ThreadX::Native::fx_system_date_set(localTime.tm_year + 1900, localTime.tm_mon + 1, localTime.tm_mday)}; error != Error::success)
    {
        return error;
    }

    if (Error error{ThreadX::Native::fx_system_time_set(localTime.tm_hour, localTime.tm_min, localTime.tm_sec)}; error != Error::success)
    {
        return error;
    }

    return Error::success;
}

template <MediaSectorSize N>
Media<N>::Media(std::byte *driverInfoPtr, const NotifyCallback &openNotifyCallback, const NotifyCallback &closeNotifyCallback)
    : ThreadX::Native::FX_MEDIA{}, m_driverInfoPtr{driverInfoPtr}, m_openNotifyCallback{openNotifyCallback}, m_closeNotifyCallback{closeNotifyCallback}
{
    if (not m_fileSystemInitialised.test_and_set())
    {
        ThreadX::Native::fx_system_initialize();
        setFileSystemTime();
    }

    if (m_openNotifyCallback)
    {
        [[maybe_unused]] Error error{fx_media_open_notify_set(this, Media::openNotifyCallback)};
        assert(error == Error::success);
    }

    if (m_closeNotifyCallback)
    {
        [[maybe_unused]] Error error{fx_media_close_notify_set(this, Media::closeNotifyCallback)};
        assert(error == Error::success);
    }
}

template <MediaSectorSize N>
Media<N>::~Media()
{
    [[maybe_unused]] auto error{close()};
    assert(error == Error::success || error == Error::mediaNotOpen);
}

template <MediaSectorSize N>
auto Media<N>::open(const std::string_view name, const FaultTolerantMode mode)
{
    using namespace ThreadX::Native;

    if (Error error{
            fx_media_open(this, const_cast<char *>(name.data()), Media::driverCallback, m_driverInfoPtr, m_mediaMemory.data(), m_mediaMemorySizeInBytes)};
        error != Error::success)
    {
        return error;
    }

    if (mode == FaultTolerantMode::enable)
    {
#ifdef FX_ENABLE_FAULT_TOLERANT
        return Error{fx_fault_tolerant_enable(this, m_faultTolerantCache.data(), m_faultTolerantCacheSize)};
#else
        assert(mode == FaultTolerantMode::disable);
#endif
    }

    return Error::success;
}

template <MediaSectorSize N>
auto Media<N>::format(const std::string_view volumeName, const ThreadX::Ulong storageSize, const ThreadX::Uint sectorPerCluster,
                      const ThreadX::Uint directoryEntriesFat12_16)
{
    assert(storageSize % std::to_underlying(N) == 0);

    return Error{
        fx_media_format(this,
                        Media::driverCallback,                                    // Driver entry
                        m_driverInfoPtr,                                          // could be RAM disk memory pointer
                        reinterpret_cast<ThreadX::Uchar *>(m_mediaMemory.data()), // Media buffer pointer
                        m_mediaMemorySizeInBytes,                                 // Media buffer size
                        const_cast<char *>(volumeName.data()),                    // Volume Name
                        1,                                                        // Number of FATs
                        directoryEntriesFat12_16,                                 // Directory Entries
                        0,                                                        // Hidden sectors
                        storageSize / std::to_underlying(N),                      // Total sectors
                        std::to_underlying(N),                                    // Sector size
                        sectorPerCluster,                                         // Sectors per cluster
                        1,                                                        // Heads
                        1)                                                        // Sectors per track
    };
}

template <MediaSectorSize N>
auto Media<N>::volume(const std::string_view volumeName)
{
    assert(volumeName.length() < volumNameLength);
    return Error{fx_media_volume_set(this, const_cast<char *>(volumeName.data()))};
}

template <MediaSectorSize N>
auto Media<N>::volume() -> ExpectedStr
{
    char volumeName[volumNameLength];
    if (Error error{fx_media_volume_get(this, volumeName, FX_BOOT_SECTOR)}; error != Error::success)
    {
        return std::unexpected(error);
    }

    return volumeName;
}

template <MediaSectorSize N>
auto Media<N>::createDir(const std::string_view dirName)
{
    return Error{fx_directory_create(this, const_cast<char *>(dirName.data()))};
}

template <MediaSectorSize N>
auto Media<N>::deleteDir(const std::string_view dirName)
{
    return Error{fx_directory_delete(this, const_cast<char *>(dirName.data()))};
}

template <MediaSectorSize N>
auto Media<N>::renameDir(const std::string_view dirName, const std::string_view newName)
{
    return Error{fx_directory_rename(this, const_cast<char *>(dirName.data()), const_cast<char *>(newName.data()))};
}

template <MediaSectorSize N>
auto Media<N>::createFile(const std::string_view fileName)
{
    return Error{fx_file_create(this, const_cast<char *>(fileName.data()))};
}

template <MediaSectorSize N>
auto Media<N>::deleteFile(const std::string_view fileName)
{
    return Error{fx_file_delete(this, const_cast<char *>(fileName.data()))};
}

template <MediaSectorSize N>
auto Media<N>::renameFile(const std::string_view fileName, const std::string_view newFileName)
{
    return Error{fx_file_rename(this, const_cast<char *>(fileName.data()), const_cast<char *>(newFileName.data()))};
}

template <MediaSectorSize N>
auto Media<N>::defaultDir(const std::string_view newPath)
{
    return Error{fx_directory_default_set(this, const_cast<char *>(newPath.data()))};
}

template <MediaSectorSize N>
auto Media<N>::defaultDir() -> ExpectedStr
{
    char *path = nullptr;
    if (Error error{fx_directory_default_get(this, std::addressof(path))}; error != Error::success)
    {
        return std::unexpected(error);
    }

    return path;
}

template <MediaSectorSize N>
auto Media<N>::localDir(const std::string_view newPath)
{
    using namespace ThreadX::Native;
    FX_LOCAL_PATH localPath;
    return Error{fx_directory_local_path_set(this, std::addressof(localPath), const_cast<char *>(newPath.data()))};
}

template <MediaSectorSize N>
auto Media<N>::localDir() -> ExpectedStr
{
    char *path = nullptr;
    if (Error error{fx_directory_local_path_get(this, std::addressof(path))}; error != Error::success)
    {
        return std::unexpected(error);
    }

    return path;
}

template <MediaSectorSize N>
auto Media<N>::clearLocalDir()
{
    return Error{fx_directory_local_path_clear(this)};
}

template <MediaSectorSize N>
auto Media<N>::space() -> ExpectedUlong64
{
    ThreadX::Ulong64 spaceLeft{};
    if (Error error{fx_media_extended_space_available(this, std::addressof(spaceLeft))}; error != Error::success)
    {
        return std::unexpected(error);
    }

    return spaceLeft;
}

template <MediaSectorSize N>
auto Media<N>::abort()
{
    return Error{fx_media_abort(this)};
}

template <MediaSectorSize N>
auto Media<N>::invalidateCache()
{
    return Error{fx_media_cache_invalidate(this)};
}

template <MediaSectorSize N>
auto Media<N>::flush()
{
    return Error{fx_media_flush(this)};
}

template <MediaSectorSize N>
auto Media<N>::close()
{
    return Error{fx_media_close(this)};
}

template <MediaSectorSize N>
auto Media<N>::name() const
{
    return std::string_view{fx_media_name};
}

template <MediaSectorSize N>
auto Media<N>::writeSector(const ThreadX::Ulong sectorNo, const std::span<std::byte, std::to_underlying(N)> sectorData)
{
    return Error{fx_media_write(this, sectorNo, sectorData.data())};
}

template <MediaSectorSize N>
auto Media<N>::readSector(const ThreadX::Ulong sectorNo, std::span<std::byte, std::to_underlying(N)> sectorData)
{
    return Error{fx_media_read(this, sectorNo, sectorData.data())};
}

template <MediaSectorSize N>
auto Media<N>::driverInfo()
{
    return fx_media_driver_info;
}

template <MediaSectorSize N>
auto Media<N>::driverRequest()
{
    return MediaDriverRequest{fx_media_driver_request};
}
template <MediaSectorSize N>
auto Media<N>::driverStatus(const Error error)
{
    fx_media_driver_status = std::to_underlying(error);
}

template <MediaSectorSize N>
auto Media<N>::driverBuffer()
{
    return fx_media_driver_buffer;
}

template <MediaSectorSize N>
auto Media<N>::driverLogicalSector()
{
    return fx_media_driver_logical_sector;
}

template <MediaSectorSize N>
auto Media<N>::driverSectors()
{
    return fx_media_driver_sectors;
}

template <MediaSectorSize N>
auto Media<N>::driverWriteProtect(const bool writeProtect)
{
    fx_media_driver_write_protect = writeProtect;
}
template <MediaSectorSize N>
auto Media<N>::driverFreeSectorUpdate(const bool freeSectorUpdate)
{
    fx_media_driver_free_sector_update = freeSectorUpdate;
}
template <MediaSectorSize N>
auto Media<N>::driverSystemWrite()
{
    return static_cast<bool>(fx_media_driver_system_write);
}
template <MediaSectorSize N>
auto Media<N>::driverDataSectorRead()
{
    return static_cast<bool>(fx_media_driver_data_sector_read);
}
template <MediaSectorSize N>
auto Media<N>::driverSectorType()
{
    return MediaSectorType{fx_media_driver_sector_type};
}

template <MediaSectorSize N>
auto Media<N>::driverCallback(auto mediaPtr)
{
    auto &media{static_cast<Media &>(*mediaPtr)};
    media.driverCallback();
}

template <MediaSectorSize N>
auto Media<N>::openNotifyCallback(auto mediaPtr)
{
    auto &media{static_cast<Media &>(*mediaPtr)};
    media.m_openNotifyCallback(media);
}

template <MediaSectorSize N>
auto Media<N>::closeNotifyCallback(auto mediaPtr)
{
    auto &media{static_cast<Media &>(*mediaPtr)};
    media.m_closeNotifyCallback(media);
}
} // namespace FileX
