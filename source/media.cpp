#include "media.hpp"
#include <cassert>

namespace FileX
{
MediaBase::MediaBase() : ThreadX::Native::FX_MEDIA{}
{
}

MediaBase::~MediaBase()
{
    [[maybe_unused]] Error error{fx_media_close(this)};
    assert(error == Error::success);
}

Error MediaBase::volume(const std::string_view volumeName)
{
    assert(volumeName.length() < volumNameLength);
    return Error{fx_media_volume_set(this, const_cast<char *>(volumeName.data()))};
}

MediaBase::StrPair MediaBase::volume()
{
    char volumeName[volumNameLength];
    Error error{fx_media_volume_get(this, volumeName, FX_BOOT_SECTOR)};
    return {error, volumeName};
}

Error MediaBase::createDir(const std::string_view dirName)
{
    return Error{fx_directory_create(this, const_cast<char *>(dirName.data()))};
}

Error MediaBase::deleteDir(const std::string_view dirName)
{
    return Error{fx_directory_delete(this, const_cast<char *>(dirName.data()))};
}

Error MediaBase::renameDir(const std::string_view dirName, const std::string_view newName)
{
    return Error{fx_directory_rename(this, const_cast<char *>(dirName.data()), const_cast<char *>(newName.data()))};
}

Error MediaBase::createFile(const std::string_view fileName)
{
    return Error{fx_file_create(this, const_cast<char *>(fileName.data()))};
}

Error MediaBase::deleteFile(const std::string_view fileName)
{
    return Error{fx_file_delete(this, const_cast<char *>(fileName.data()))};
}

Error MediaBase::renameFile(const std::string_view fileName, const std::string_view newFileName)
{
    return Error{fx_file_rename(this, const_cast<char *>(fileName.data()), const_cast<char *>(newFileName.data()))};
}

Error MediaBase::defaultDir(const std::string_view newPath)
{
    return Error{fx_directory_default_set(this, const_cast<char *>(newPath.data()))};
}

MediaBase::StrPair MediaBase::defaultDir()
{
    char *path = nullptr;
    Error error{fx_directory_default_get(this, std::addressof(path))};
    return {error, path};
}

Error MediaBase::localDir(const std::string_view newPath)
{
    using namespace ThreadX::Native;
    FX_LOCAL_PATH localPath;
    return Error{fx_directory_local_path_set(this, std::addressof(localPath), const_cast<char *>(newPath.data()))};
}

MediaBase::StrPair MediaBase::localDir()
{
    char *path = nullptr;
    Error error{fx_directory_local_path_get(this, std::addressof(path))};
    return {error, path};
}

Error MediaBase::clearLocalDir()
{
    return Error{fx_directory_local_path_clear(this)};
}

MediaBase::Ulong64Pair MediaBase::space()
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

std::string_view MediaBase::name() const
{
    return std::string_view(fx_media_name);
}
} // namespace FileX
