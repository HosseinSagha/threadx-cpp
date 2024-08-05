#pragma once

#include "fxCommon.hpp"
#include "media.hpp"
#include <cstring>
#include <span>
#include <string_view>

namespace FileX
{
enum class OpenOption : ThreadX::Uint
{
    read = FX_OPEN_FOR_READ,
    write = FX_OPEN_FOR_WRITE,
    fastRead = FX_OPEN_FOR_READ_FAST,
};

enum class SeekFrom : ThreadX::Uint
{
    begin = FX_SEEK_BEGIN,
    end = FX_SEEK_END,
    forward = FX_SEEK_FORWARD,
    back = FX_SEEK_BACK
};

enum class AllocateOption
{
    strict,
    bestEffort
};

enum class TruncateOption
{
    noRelease,
    release
};

class File : ThreadX::Native::FX_FILE
{
  public:
    using UlongPair = std::pair<Error, ThreadX::Ulong>;
    using NotifyCallback = std::function<void(File &)>;

    explicit File(const std::string_view fileName, MediaBase &media, const OpenOption option = OpenOption::read,
         NotifyCallback writeNotifyCallback = {});
    ~File();
    MediaBase::Ulong64Pair allocate(ThreadX::Ulong64 size, AllocateOption option = AllocateOption::strict);
    Error truncate(ThreadX::Ulong64 newSize, TruncateOption option = TruncateOption::noRelease);
    Error seek(const ThreadX::Ulong64 offset);
    Error relativeSeek(const ThreadX::Ulong64 offset, const SeekFrom from = SeekFrom::forward);
    Error write(const std::span<std::byte> data);
    Error write(const std::string_view str);
    UlongPair read(std::span<std::byte> buffer);
    UlongPair read(std::span<std::byte> buffer, const ThreadX::Ulong size);
    Error close();

  private:
    static void writeNotifyCallback(auto notifyFilePtr);

    const NotifyCallback m_writeNotifyCallback;
};
} // namespace FileX