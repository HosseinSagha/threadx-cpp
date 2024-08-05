#pragma once

#include "fxCommon.hpp"
#include "tickTimer.hpp"
#include <array>
#include <atomic>
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

enum class SectorSize : ThreadX::Uint
{
    halfAKilobyte = 512,
    oneKiloByte = 1024,
    twoKiloBytes = 2048,
    fourKilobytes = 4096
};

auto constexpr defaultSectorSize{SectorSize::halfAKilobyte};

class MediaBase : protected ThreadX::Native::FX_MEDIA
{
  public:
    using Ulong64Pair = std::pair<Error, ThreadX::Ulong64>;
    using StrPair = std::pair<Error, std::string_view>;

    friend class File;

    MediaBase(const MediaBase &) = delete;
    MediaBase &operator=(const MediaBase &) = delete;

    template <class Clock, typename Duration>
    static auto fileSystemTime(const std::chrono::time_point<Clock, Duration> &time);

    Error volume(const std::string_view volumeName);
    StrPair volume();
    Error createDir(const std::string_view dirName);
    Error deleteDir(const std::string_view dirName);
    Error renameDir(const std::string_view dirName, const std::string_view newName);
    Error createFile(const std::string_view fileName);
    Error deleteFile(const std::string_view fileName);
    Error renameFile(const std::string_view fileName, const std::string_view newFileName);
    Error defaultDir(const std::string_view newPath);
    StrPair defaultDir();
    Error localDir(const std::string_view newPath);
    StrPair localDir();
    Error clearLocalDir();

    Ulong64Pair space();

    /// This service is typically called when I/O errors are detected
    Error abort();

    Error invalidateCache();

    Error check();

    Error flush();

    Error close();
    std::string_view name() const;

  protected:
    static inline std::atomic_flag m_fileSystemInitialised = ATOMIC_FLAG_INIT;

    explicit MediaBase();
    ~MediaBase();

  private:
    static constexpr size_t volumNameLength{12};
};

template <class Clock, typename Duration>
auto MediaBase::fileSystemTime(const std::chrono::time_point<Clock, Duration> &time)
{
    auto [localTime, frac_ms]{ThreadX::TickTimer::to_localtime(
        std::chrono::time_point_cast<ThreadX::TickTimer, ThreadX::TickTimer::Duration>(time))};

    if (Error error{
            ThreadX::Native::fx_system_date_set(localTime.tm_year + 1900, localTime.tm_mon + 1, localTime.tm_mday)};
        error != Error::success)
    {
        return error;
    }

    if (Error error{ThreadX::Native::fx_system_time_set(localTime.tm_hour, localTime.tm_min, localTime.tm_sec)};
        error != Error::success)
    {
        return error;
    }

    return Error::success;
}

template <SectorSize N = defaultSectorSize> class Media : public MediaBase
{
  public:
    using DriverCallback = std::function<void(ThreadX::Native::FX_MEDIA *mediaPtr)>;
    using NotifyCallback = std::function<void(Media &)>;

    constexpr SectorSize sectorSize() const;
    //Once initialized by this constructor, the application should call fx_system_date_set and fx_system_time_set to start with an accurate system date and time.
    explicit Media(const DriverCallback &driverCallback, void *driverInfoPtr = nullptr,
                   const NotifyCallback &openNotifyCallback = {}, const NotifyCallback &closeNotifyCallback = {});

    Media(const Media &) = delete;
    Media &operator=(const Media &) = delete;

    auto open(const std::string_view name, const FaultTolerantMode mode = FaultTolerantMode::enable);
    auto format(const std::string_view volumeName, const ThreadX::Ulong storageSize,
                const ThreadX::Uint sectorPerCluster = 1, const ThreadX::Uint directoryEntries = 32);
    auto writeSector(const ThreadX::Ulong sectorNo, const std::span<std::byte, std::to_underlying(N)> sectorData);
    auto readSector(const ThreadX::Ulong sectorNo, std::span<std::byte, std::to_underlying(N)> sectorData);

  private:
    using MediaBase::m_fileSystemInitialised;

    static auto driverCallback(auto mediaPtr);
    static auto openNotifyCallback(auto mediaPtr);
    static auto closeNotifyCallback(auto mediaPtr);

#ifdef FX_ENABLE_FAULT_TOLERANT
    static constexpr ThreadX::Uint faultTolerantCacheSize{FX_FAULT_TOLERANT_MAXIMUM_LOG_FILE_SIZE};
    static_assert(
        faultTolerantCacheSize % ThreadX::wordSize == 0, "Fault tolerant cache size must be a multiple of word size.");
    // the scratch memory size shall be at least 3072 bytes and must be multiple of sector size.
    static constexpr auto cacheSize = []() {
        return (N == SectorSize::twoKiloBytes or N == SectorSize::fourKilobytes)
                   ? std::to_underlying(SectorSize::fourKilobytes) / ThreadX::wordSize
                   : faultTolerantCacheSize / ThreadX::wordSize;
    };
    std::array<ThreadX::Ulong, cacheSize()> m_faultTolerantCache{};
#endif
    const DriverCallback m_driverCallback;
    void *m_driverInfoPtr;
    const NotifyCallback m_openNotifyCallback;
    const NotifyCallback m_closeNotifyCallback;
    std::array<ThreadX::Uchar, std::to_underlying(N)> m_mediaMemory{};
};

template <SectorSize N> constexpr SectorSize Media<N>::sectorSize() const
{
    return N;
}

template <SectorSize N>
Media<N>::Media(const DriverCallback &driverCallback, void *driverInfoPtr, const NotifyCallback &openNotifyCallback,
                const NotifyCallback &closeNotifyCallback)
    : m_driverCallback{driverCallback}, m_driverInfoPtr{driverInfoPtr}, m_openNotifyCallback{openNotifyCallback},
      m_closeNotifyCallback{closeNotifyCallback}
{
    if (not m_fileSystemInitialised.test_and_set())
    {
        ThreadX::Native::fx_system_initialize();
    }

    if (m_closeNotifyCallback)
    {
        [[maybe_unused]] Error error{fx_media_close_notify_set(this, Media::closeNotifyCallback)};
        assert(error == Error::success);
    }

    if (m_openNotifyCallback)
    {
        [[maybe_unused]] Error error{fx_media_open_notify_set(this, Media::openNotifyCallback)};
        assert(error == Error::success);
    }
}

template <SectorSize N> auto Media<N>::open(const std::string_view name, const FaultTolerantMode mode)
{
    using namespace ThreadX::Native;

    if (Error error{fx_media_open(this, const_cast<char *>(name.data()), Media::driverCallback, m_driverInfoPtr,
                                  m_mediaMemory.data(), m_mediaMemory.size())};
        error != Error::success)
    {
        return error;
    }

    if (mode == FaultTolerantMode::enable)
    {
#ifdef FX_ENABLE_FAULT_TOLERANT
        return Error{fx_fault_tolerant_enable(this, m_faultTolerantCache.data(), faultTolerantCacheSize)};
#else
        assert(mode == FaultTolerantMode::disable);
#endif
    }

    return Error::success;
}

template <SectorSize N>
auto Media<N>::format(const std::string_view volumeName, const ThreadX::Ulong storageSize,
                      const ThreadX::Uint sectorPerCluster, const ThreadX::Uint directoryEntriesFat12_16)
{
    assert(storageSize % std::to_underlying(N) == 0);

    return Error{
        fx_media_format(
            this,
            Media::driverCallback,                 // Driver entry
            m_driverInfoPtr,                       // could be RAM disk memory pointer
            m_mediaMemory.data(),                  // Media buffer pointer
            m_mediaMemory.size(),                  // Media buffer size
            const_cast<char *>(volumeName.data()), // Volume Name
            1,                                     // Number of FATs
            directoryEntriesFat12_16,              // Directory Entries
            0,                                     // Hidden sectors
            storageSize / std::to_underlying(N),   // Total sectors
            std::to_underlying(N),                 // Sector size
            sectorPerCluster,                      // Sectors per cluster
            1,                                     // Heads
            1)                                     // Sectors per track
    };
}

template <SectorSize N>
auto Media<N>::writeSector(const ThreadX::Ulong sectorNo, const std::span<std::byte, std::to_underlying(N)> sectorData)
{
    return Error{fx_media_write(this, sectorNo, sectorData.data())};
}

template <SectorSize N>
auto Media<N>::readSector(const ThreadX::Ulong sectorNo, std::span<std::byte, std::to_underlying(N)> sectorData)
{
    return Error{fx_media_read(this, sectorNo, sectorData.data())};
}

template <SectorSize N> auto Media<N>::driverCallback(auto mediaPtr)
{
    auto &media{static_cast<Media &>(*mediaPtr)};
    media.m_driverCallback(mediaPtr);
}

template <SectorSize N> auto Media<N>::openNotifyCallback(auto mediaPtr)
{
    auto &media{static_cast<Media &>(*mediaPtr)};
    media.m_openNotifyCallback(media);
}

template <SectorSize N> auto Media<N>::closeNotifyCallback(auto mediaPtr)
{
    auto &media{static_cast<Media &>(*mediaPtr)};
    media.m_closeNotifyCallback(media);
}
} // namespace FileX
