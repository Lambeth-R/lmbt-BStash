#pragma once

#include <Windows.h>
#if !_HAS_CXX17
#define INCLUDE_STD_FILESYSTEM_EXPERIMENTAL 1
#define _SILENCE_EXPERIMENTAL_FILESYSTEM_DEPRECATION_WARNING
#include <experimental/filesystem>
#else
#include <filesystem>
#endif
#include <vector>
#include <string>

namespace fs 
{
#if _HAS_CXX17
    using path = std::filesystem::path;
#else
    using path = std::experimental::filesystem::v1::path;
#endif

    struct File_Metadata
    {
        FILETIME CreationTime;
        FILETIME LastAccessTime;
        FILETIME LastWriteTime;
        FILETIME ChangeTime;
        DWORD FileAttributes;
    };
}

const fs::path Fs_GetMountPointPath(const fs::path& path);

const fs::path Fs_ExpandPath(const fs::path& path);

bool Fs_WriteFile(const fs::path& filePath, const std::string& data, bool force = false);

bool Fs_ReadFile(const fs::path& filePath, std::string& data);

bool Fs_AppendFile(const fs::path& filePath, const std::string& data);

bool Fs_DeleteFile(const fs::path& filePath);

bool Fs_RemoveDir(const fs::path& path, bool recursive = true);

bool Fs_IsDirectory(const fs::path& path);

bool Fs_IsExist(const fs::path& path);

bool Fs_CreateDirectory(const fs::path& path, bool recirsive = true);

bool Fs_GetFileMetadata(const fs::path& path, fs::File_Metadata& attributes);

bool Fs_SetFileMetadata(const fs::path& path, const fs::File_Metadata& attributes);

const std::vector<fs::path> Fs_EnumDir(const fs::path& path); // add filter