#include "media.hpp"

namespace FileX
{
Error MediaBase::fileSystemTime(const ThreadX::TickTimer::TimePoint &time)
{
    auto t{ThreadX::TickTimer::to_time_t(time)};
    auto localTime{std::localtime(std::addressof(t))};

    if (Error error{
            ThreadX::Native::fx_system_date_set(localTime->tm_year + 1900, localTime->tm_mon + 1, localTime->tm_mday)};
        error != Error::success)
    {
        return error;
    }

    if (Error error{ThreadX::Native::fx_system_time_set(localTime->tm_hour, localTime->tm_min, localTime->tm_sec)};
        error != Error::success)
    {
        return error;
    }

    return Error::success;
}

MediaBase::MediaBase() : ThreadX::Native::FX_MEDIA{}
{
}

MediaBase::~MediaBase()
{
    fx_media_close(this);
}

Error MediaBase::volume(std::string_view volumeName)
{
    assert(volumeName.length() < volumNameLength);
    return Error{fx_media_volume_set(this, const_cast<char *>(volumeName.data()))};
}

MediaBase::ReturnTupleStr MediaBase::volume()
{
    char volumeName[volumNameLength];
    Error error{fx_media_volume_get(this, volumeName, FX_BOOT_SECTOR)};
    return {error, volumeName};
}

Error MediaBase::createDir(std::string_view dirName)
{
    return Error{fx_directory_create(this, const_cast<char *>(dirName.data()))};
}

Error MediaBase::deleteDir(std::string_view dirName)
{
    return Error{fx_directory_delete(this, const_cast<char *>(dirName.data()))};
}

Error MediaBase::renameDir(std::string_view dirName, std::string_view newName)
{
    return Error{fx_directory_rename(this, const_cast<char *>(dirName.data()), const_cast<char *>(newName.data()))};
}

Error MediaBase::createFile(std::string_view fileName)
{
    return Error{fx_file_create(this, const_cast<char *>(fileName.data()))};
}

Error MediaBase::deleteFile(std::string_view fileName)
{
    return Error{fx_file_delete(this, const_cast<char *>(fileName.data()))};
}

Error MediaBase::renameFile(std::string_view fileName, std::string_view newFileName)
{
    return Error{fx_file_rename(this, const_cast<char *>(fileName.data()), const_cast<char *>(newFileName.data()))};
}

Error MediaBase::defaultDir(std::string_view newPath)
{
    return Error{fx_directory_default_set(this, const_cast<char *>(newPath.data()))};
}

MediaBase::ReturnTupleStr MediaBase::defaultDir()
{
    char *path = nullptr;
    Error error{fx_directory_default_get(this, std::addressof(path))};
    return {error, path};
}

Error MediaBase::localDir(std::string_view newPath)
{
    using namespace ThreadX::Native;
    FX_LOCAL_PATH localPath;
    return Error{fx_directory_local_path_set(this, std::addressof(localPath), const_cast<char *>(newPath.data()))};
}

MediaBase::ReturnTupleStr MediaBase::localDir()
{
    char *path = nullptr;
    Error error{fx_directory_local_path_get(this, std::addressof(path))};
    return {error, path};
}

Error MediaBase::clearLocalDir()
{
    return Error{fx_directory_local_path_clear(this)};
}

MediaBase::ReturnTuple MediaBase::space()
{
    ThreadX::Ulong64 spaceLeft{};
    Error error{fx_media_extended_space_available(this, std::addressof(spaceLeft))};
    return {error, spaceLeft};
}

Error MediaBase::abort()
{
    return Error{fx_media_abort(this)};
}

Error MediaBase::invalidateCache()
{
    return Error{fx_media_cache_invalidate(this)};
}

Error MediaBase::flush()
{
    return Error{fx_media_flush(this)};
}

Error MediaBase::close()
{
    return Error{fx_media_close(this)};
}
} // namespace FileX
