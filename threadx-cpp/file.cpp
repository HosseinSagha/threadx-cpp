#include "file.hpp"
#include "media.hpp"
#include <cassert>
#include <expected>
#include <utility>

namespace FileX
{
File::~File()
{
    [[maybe_unused]] Error error{fx_file_close(this)};
    assert(error == Error::success);
}

auto File::allocate(const ThreadX::Ulong64 size, const AllocateOption option) -> ExpectedUlong64
{
    Error error{};

    if (option == AllocateOption::strict)
    {
        if (error = Error{fx_file_extended_allocate(this, size)}; error == Error::success)
        {
            return size;
        }
    }
    else
    {
        ThreadX::Ulong64 allocatedSize{};
        if (error = Error{fx_file_extended_best_effort_allocate(this, size, std::addressof(allocatedSize))};
            error == Error::success)
        {
            return allocatedSize;
        }
    }

    return std::unexpected(error);
}

auto File::truncate(const ThreadX::Ulong64 newSize, const TruncateOption option) -> Error
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

auto File::seek(const ThreadX::Ulong64 offset) -> Error
{
    return Error{fx_file_extended_seek(this, offset)};
}

auto File::relativeSeek(const ThreadX::Ulong64 offset, const SeekFrom from) -> Error
{
    return Error{fx_file_extended_relative_seek(this, offset, std::to_underlying(from))};
}

auto File::write(const std::span<std::byte> data) -> Error
{
    return Error{fx_file_write(this, data.data(), data.size())};
}

auto File::write(const std::string_view str) -> Error
{
    return Error{fx_file_write(this, const_cast<char *>(str.data()), str.size())};
}

auto File::read(const std::span<std::byte> buffer) -> ExpectedUlong
{
    ThreadX::Ulong actualSize{};
    if (Error error{fx_file_read(this, buffer.data(), buffer.size(), std::addressof(actualSize))};
        error != Error::success)
    {
        return std::unexpected(error);
    }

    return actualSize;
}

auto File::read(const std::span<std::byte> buffer, const ThreadX::Ulong size) -> ExpectedUlong
{
    assert(size <= buffer.size());

    ThreadX::Ulong actualSize{};
    if (Error error{fx_file_read(this, buffer.data(), size, std::addressof(actualSize))}; error != Error::success)
    {
        return std::unexpected(error);
    }

    return actualSize;
}

auto File::writeNotifyCallback(ThreadX::Native::FX_FILE *notifyFilePtr) -> void
{
    auto &file{static_cast<File &>(*notifyFilePtr)};
    file.m_writeNotifyCallback(file);
}
} // namespace FileX
