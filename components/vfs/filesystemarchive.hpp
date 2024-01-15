#ifndef OPENMW_COMPONENTS_RESOURCE_FILESYSTEMARCHIVE_H
#define OPENMW_COMPONENTS_RESOURCE_FILESYSTEMARCHIVE_H

#include "archive.hpp"
#include "file.hpp"

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

        void listResources(FileMap& out) override;

        bool contains(std::string_view file) const override;

        std::string getDescription() const override;

    private:
        std::map<std::string, FileSystemArchiveFile, std::less<>> mIndex;
        bool mBuiltIndex;
        std::filesystem::path mPath;
    };

}

#endif
