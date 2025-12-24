#pragma once

#include "fxCommon.hpp"
#include <cassert>
#include <cstring>
#include <expected>
#include <functional>
#include <span>
#include <string_view>
#include <utility>

namespace FileX
{
template <MediaSectorSize N> class Media;

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

class File final : ThreadX::Native::FX_FILE
{
  public:
    using ExpectedUlong = std::expected<ThreadX::Ulong, Error>;
    using ExpectedUlong64 = std::expected<ThreadX::Ulong64, Error>;
    using NotifyCallback = std::function<void(File &)>;

    template <MediaSectorSize N>
    explicit File(const std::string_view fileName,
                  Media<N> &media,
                  const OpenOption option = OpenOption::read,
                  const NotifyCallback writeNotifyCallback = {});
    ~File();

    auto allocate(const ThreadX::Ulong64 size, const AllocateOption option = AllocateOption::strict) -> ExpectedUlong64;
    auto truncate(const ThreadX::Ulong64 newSize, const TruncateOption option = TruncateOption::noRelease) -> Error;
    auto seek(const ThreadX::Ulong64 offset) -> Error;
    auto relativeSeek(const ThreadX::Ulong64 offset, const SeekFrom from = SeekFrom::forward) -> Error;
    auto write(const std::span<std::byte> data) -> Error;
    auto write(const std::string_view str) -> Error;
    auto read(const std::span<std::byte> buffer) -> ExpectedUlong;
    auto read(const std::span<std::byte> buffer, const ThreadX::Ulong size) -> ExpectedUlong;

  private:
    static auto writeNotifyCallback(ThreadX::Native::FX_FILE *notifyFilePtr) -> void;

    const NotifyCallback m_writeNotifyCallback;
};

template <MediaSectorSize N>
File::File(const std::string_view fileName,
           Media<N> &media,
           const OpenOption option,
           const NotifyCallback writeNotifyCallback)
    : ThreadX::Native::FX_FILE{},
      m_writeNotifyCallback{std::move(writeNotifyCallback)}
{
    using namespace ThreadX::Native;
    [[maybe_unused]] Error error{
        fx_file_open(std::addressof(media), this, const_cast<char *>(fileName.data()), std::to_underlying(option))};
    assert(error == Error::success);

    if (m_writeNotifyCallback)
    {
        error = Error{fx_file_write_notify_set(this, File::writeNotifyCallback)};
        assert(error == Error::success);
    }
}
} // namespace FileX
