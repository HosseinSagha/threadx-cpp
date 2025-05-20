#pragma once

#include "fxCommon.hpp"
#include "tickTimer.hpp"
#include <array>
#include <expected>
#include <functional>
#include <mutex>
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

    static inline std::once_flag m_initialisedFlag{};
};

template <MediaSectorSize N = defaultSectorSize>
class Media : ThreadX::Native::FX_MEDIA, MediaBase
{
  public:
    friend class File;

    class InternalDriver
    {
      public:
        InternalDriver(Media &media);

        [[nodiscard]] static consteval auto sectorSize() -> MediaSectorSize;
        auto info() const -> void *;
        auto request() const -> MediaDriverRequest;
        auto status(const Error error) -> void;
        auto buffer() const -> ThreadX::Uchar *;
        auto logicalSector() const -> ThreadX::Ulong;
        auto sectors() const -> ThreadX::Ulong;
        auto physicalSector() -> ThreadX::Ulong;
        auto physicalTrack() -> ThreadX::Uint;
        auto writeProtect(const bool writeProtect = true) -> void;
        auto freeSectorUpdate(const bool freeSectorUpdate = true) -> void;
        auto systemWrite() const -> bool;
        auto dataSectorRead() const -> bool;
        auto sectorType() const -> MediaSectorType;

      private:
        Media &m_media;
    };

    using DriverCallback = std::function<void(InternalDriver &)>;
    using NotifyCallback = std::function<void(Media &)>;
    using ExpectedUlong64 = std::expected<ThreadX::Ulong64, Error>;
    using ExpectedStr = std::expected<std::string_view, Error>;

    Media(const Media &) = delete;
    Media &operator=(const Media &) = delete;

    static auto setFileSystemTime() -> Error;

    // Once initialized by this constructor, the application should call fx_system_date_set and fx_system_time_set to start with an accurate system date and
    // time.
    explicit Media(const DriverCallback &driverCallback, std::byte *driverInfoPtr = nullptr, const NotifyCallback &openNotifyCallback = {},
                   const NotifyCallback &closeNotifyCallback = {});
    virtual ~Media();

    auto open(const std::string_view name, const FaultTolerantMode mode = FaultTolerantMode::enable) -> Error;
    auto format(const std::string_view volumeName, const ThreadX::Ulong storageSize, const ThreadX::Uint sectorPerCluster = 1,
                const ThreadX::Uint directoryEntriesFat12_16 = 32) -> Error;
    auto volume(const std::string_view volumeName) -> Error;
    [[nodiscard]] auto volume() -> ExpectedStr;
    auto createDir(const std::string_view dirName) -> Error;
    auto deleteDir(const std::string_view dirName) -> Error;
    auto renameDir(const std::string_view dirName, const std::string_view newName) -> Error;
    auto createFile(const std::string_view fileName) -> Error;
    auto deleteFile(const std::string_view fileName) -> Error;
    auto renameFile(const std::string_view fileName, const std::string_view newFileName) -> Error;
    auto defaultDir(const std::string_view newPath) -> Error;
    [[nodiscard]] auto defaultDir() -> ExpectedStr;
    auto localDir(const std::string_view newPath) -> Error;
    [[nodiscard]] auto localDir() -> ExpectedStr;
    auto clearLocalDir() -> Error;
    [[nodiscard]] auto space() -> ExpectedUlong64;
    /// This service is typically called when I/O errors are detected
    auto abort() -> Error;
    auto invalidateCache() -> Error;
    auto check() -> Error;
    auto flush() -> Error;
    auto close() -> Error;
    [[nodiscard]] auto name() const -> std::string_view;
    auto writeSector(const ThreadX::Ulong sectorNo, const std::span<std::byte, std::to_underlying(N)> sectorData) -> Error;
    auto readSector(const ThreadX::Ulong sectorNo, std::span<std::byte, std::to_underlying(N)> sectorData) -> Error;

  private:
    static auto driverCallback(auto mediaPtr) -> void;
    static auto openNotifyCallback(auto mediaPtr) -> void;
    static auto closeNotifyCallback(auto mediaPtr) -> void;

    static constexpr size_t volumNameLength{12};
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
    DriverCallback m_driverCallback;
    std::byte *m_driverInfoPtr;
    const NotifyCallback m_openNotifyCallback;
    const NotifyCallback m_closeNotifyCallback;
    InternalDriver m_internalDriver{*this};
    std::array<ThreadX::Ulong, std::to_underlying(N) / ThreadX::wordSize> m_mediaMemory{};
    const ThreadX::Ulong m_mediaMemorySizeInBytes{m_mediaMemory.size() * ThreadX::wordSize};
};

template <MediaSectorSize N>
auto Media<N>::setFileSystemTime() -> Error
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
Media<N>::Media(const DriverCallback &driverCallback, std::byte *driverInfoPtr, const NotifyCallback &openNotifyCallback,
                const NotifyCallback &closeNotifyCallback)
    : ThreadX::Native::FX_MEDIA{}, m_driverCallback{std::move(driverCallback)}, m_driverInfoPtr{driverInfoPtr},
      m_openNotifyCallback{std::move(openNotifyCallback)}, m_closeNotifyCallback{std::move(closeNotifyCallback)}
{
    std::call_once(m_initialisedFlag, []() {
        ThreadX::Native::fx_system_initialize();
        setFileSystemTime();
    });

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
    assert(error == Error::success or error == Error::mediaNotOpen);
}

template <MediaSectorSize N>
auto Media<N>::open(const std::string_view name, const FaultTolerantMode mode) -> Error
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
                      const ThreadX::Uint directoryEntriesFat12_16) -> Error
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
auto Media<N>::volume(const std::string_view volumeName) -> Error
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
auto Media<N>::createDir(const std::string_view dirName) -> Error
{
    return Error{fx_directory_create(this, const_cast<char *>(dirName.data()))};
}

template <MediaSectorSize N>
auto Media<N>::deleteDir(const std::string_view dirName) -> Error
{
    return Error{fx_directory_delete(this, const_cast<char *>(dirName.data()))};
}

template <MediaSectorSize N>
auto Media<N>::renameDir(const std::string_view dirName, const std::string_view newName) -> Error
{
    return Error{fx_directory_rename(this, const_cast<char *>(dirName.data()), const_cast<char *>(newName.data()))};
}

template <MediaSectorSize N>
auto Media<N>::createFile(const std::string_view fileName) -> Error
{
    return Error{fx_file_create(this, const_cast<char *>(fileName.data()))};
}

template <MediaSectorSize N>
auto Media<N>::deleteFile(const std::string_view fileName) -> Error
{
    return Error{fx_file_delete(this, const_cast<char *>(fileName.data()))};
}

template <MediaSectorSize N>
auto Media<N>::renameFile(const std::string_view fileName, const std::string_view newFileName) -> Error
{
    return Error{fx_file_rename(this, const_cast<char *>(fileName.data()), const_cast<char *>(newFileName.data()))};
}

template <MediaSectorSize N>
auto Media<N>::defaultDir(const std::string_view newPath) -> Error
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
auto Media<N>::localDir(const std::string_view newPath) -> Error
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
auto Media<N>::clearLocalDir() -> Error
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
auto Media<N>::abort() -> Error
{
    return Error{fx_media_abort(this)};
}

template <MediaSectorSize N>
auto Media<N>::invalidateCache() -> Error
{
    return Error{fx_media_cache_invalidate(this)};
}

template <MediaSectorSize N>
auto Media<N>::flush() -> Error
{
    return Error{fx_media_flush(this)};
}

template <MediaSectorSize N>
auto Media<N>::close() -> Error
{
    return Error{fx_media_close(this)};
}

template <MediaSectorSize N>
auto Media<N>::name() const -> std::string_view
{
    return std::string_view{fx_media_name};
}

template <MediaSectorSize N>
auto Media<N>::writeSector(const ThreadX::Ulong sectorNo, const std::span<std::byte, std::to_underlying(N)> sectorData) -> Error
{
    return Error{fx_media_write(this, sectorNo, sectorData.data())};
}

template <MediaSectorSize N>
auto Media<N>::readSector(const ThreadX::Ulong sectorNo, std::span<std::byte, std::to_underlying(N)> sectorData) -> Error
{
    return Error{fx_media_read(this, sectorNo, sectorData.data())};
}

template <MediaSectorSize N>
Media<N>::InternalDriver::InternalDriver(Media &media) : m_media{media}
{
}

template <MediaSectorSize N>
consteval auto Media<N>::InternalDriver::sectorSize() -> MediaSectorSize
{
    return N;
}

template <MediaSectorSize N>
auto Media<N>::InternalDriver::info() const -> void *
{
    // Access the member via the containing FX_MEDIA pointer
    return m_media.fx_media_driver_info;
}

template <MediaSectorSize N>
auto Media<N>::InternalDriver::request() const -> MediaDriverRequest
{
    return MediaDriverRequest{m_media.fx_media_driver_request};
}
template <MediaSectorSize N>
auto Media<N>::InternalDriver::status(const Error error) -> void
{
    m_media.fx_media_driver_status = std::to_underlying(error);
}

template <MediaSectorSize N>
auto Media<N>::InternalDriver::buffer() const -> ThreadX::Uchar *
{
    return m_media.fx_media_driver_buffer;
}

template <MediaSectorSize N>
auto Media<N>::InternalDriver::logicalSector() const -> ThreadX::Ulong
{
    return m_media.fx_media_driver_logical_sector;
}

template <MediaSectorSize N>
auto Media<N>::InternalDriver::sectors() const -> ThreadX::Ulong
{
    return m_media.fx_media_driver_sectors;
}

template <MediaSectorSize N>
auto Media<N>::InternalDriver::writeProtect(const bool writeProtect) -> void
{
    m_media.fx_media_driver_write_protect = writeProtect;
}

template <MediaSectorSize N>
auto Media<N>::InternalDriver::freeSectorUpdate(const bool freeSectorUpdate) -> void
{
    m_media.fx_media_driver_free_sector_update = freeSectorUpdate;
}

template <MediaSectorSize N>
auto Media<N>::InternalDriver::systemWrite() const -> bool
{
    return static_cast<bool>(m_media.fx_media_driver_system_write);
}

template <MediaSectorSize N>
auto Media<N>::InternalDriver::dataSectorRead() const -> bool
{
    return static_cast<bool>(m_media.fx_media_driver_data_sector_read);
}

template <MediaSectorSize N>
auto Media<N>::InternalDriver::sectorType() const -> MediaSectorType
{
    return MediaSectorType{m_media.fx_media_driver_sector_type};
}

template <MediaSectorSize N>
auto Media<N>::driverCallback(auto mediaPtr) -> void
{
    auto &media{static_cast<Media &>(*mediaPtr)};
    media.m_driverCallback(media.m_internalDriver);
}

template <MediaSectorSize N>
auto Media<N>::openNotifyCallback(auto mediaPtr) -> void
{
    auto &media{static_cast<Media &>(*mediaPtr)};
    media.m_openNotifyCallback(media);
}

template <MediaSectorSize N>
auto Media<N>::closeNotifyCallback(auto mediaPtr) -> void
{
    auto &media{static_cast<Media &>(*mediaPtr)};
    media.m_closeNotifyCallback(media);
}
} // namespace FileX
