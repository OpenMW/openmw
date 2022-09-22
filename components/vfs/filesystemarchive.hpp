#ifndef OPENMW_COMPONENTS_RESOURCE_FILESYSTEMARCHIVE_H
#define OPENMW_COMPONENTS_RESOURCE_FILESYSTEMARCHIVE_H

#include "archive.hpp"
#include <filesystem>

#include <string>

namespace VFS
{

    class FileSystemArchiveFile : public File
    {
    public:
        FileSystemArchiveFile(const std::filesystem::path& path);

        Files::IStreamPtr open() override;

        std::filesystem::path getPath() override { return mPath; }

    private:
        std::filesystem::path mPath;
    };

    class FileSystemArchive : public Archive
    {
    public:
        FileSystemArchive(const std::filesystem::path& path);

        void listResources(std::map<std::string, File*>& out, char (*normalize_function)(char)) override;

        bool contains(const std::string& file, char (*normalize_function)(char)) const override;

        std::string getDescription() const override;

    private:
        typedef std::map<std::string, FileSystemArchiveFile> index;
        index mIndex;

        bool mBuiltIndex;
        std::filesystem::path mPath;
    };

}

#endif
