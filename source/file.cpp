#include "file.hpp"
#include "media.hpp"
#include <utility>

namespace FileX
{
File::File(
    const std::string_view fileName, MediaBase &media, const OpenOption option, NotifyCallback writeNotifyCallback)
    : ThreadX::Native::FX_FILE{}, m_writeNotifyCallback{writeNotifyCallback}
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

File::~File()
{
    [[maybe_unused]] Error error{fx_file_close(this)};
    assert(error == Error::success);
}

File::Ulong64Pair File::allocate(ThreadX::Ulong64 size, AllocateOption option)
{
    Error error{};
    ThreadX::Ulong64 allocatedSize{};

    if (option == AllocateOption::strict)
    {
        if (error = Error{fx_file_extended_allocate(this, size)}; error == Error::success)
        {
            allocatedSize = size;
        }
    }
    else
    {
        error = Error{fx_file_extended_best_effort_allocate(this, size, std::addressof(allocatedSize))};
    }

    return {error, allocatedSize};
}

Error File::truncate(ThreadX::Ulong64 newSize, TruncateOption option)
{
    if (option == TruncateOption::noRelease)
    {
        return Error{fx_file_extended_truncate(this, newSize)};
    }
    else
    {
        return Error{fx_file_extended_truncate_release(this, newSize)};
    }
}

Error File::seek(const ThreadX::Ulong64 offset)
{
    return Error{fx_file_extended_seek(this, offset)};
}

Error File::relativeSeek(const ThreadX::Ulong64 offset, const SeekFrom from)
{
    return Error{fx_file_extended_relative_seek(this, offset, std::to_underlying(from))};
}

Error File::write(const std::span<std::byte> data)
{
    return Error{fx_file_write(this, data.data(), data.size())};
}

Error File::write(const std::string_view str)
{
    return Error{fx_file_write(this, const_cast<char *>(str.data()), str.size())};
}

File::UlongPair File::read(std::span<std::byte> buffer)
{
    ThreadX::Ulong actualSize{};
    Error error{fx_file_read(this, buffer.data(), buffer.size(), std::addressof(actualSize))};
    return {error, actualSize};
}

File::UlongPair File::read(std::span<std::byte> buffer, const ThreadX::Ulong size)
{
    assert(size <= buffer.size());

    ThreadX::Ulong actualSize{};
    Error error{fx_file_read(this, buffer.data(), size, std::addressof(actualSize))};
    return {error, actualSize};
}

void File::writeNotifyCallback(auto notifyFilePtr)
{
    auto &file{static_cast<File &>(*notifyFilePtr)};
    file.m_writeNotifyCallback(file);
}
} // namespace FileX
