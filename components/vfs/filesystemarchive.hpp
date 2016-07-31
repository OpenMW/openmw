#ifndef OPENMW_COMPONENTS_RESOURCE_FILESYSTEMARCHIVE_H
#define OPENMW_COMPONENTS_RESOURCE_FILESYSTEMARCHIVE_H

#include "archive.hpp"

namespace VFS
{

    class FileSystemArchiveFile : public File
    {
    public:
        FileSystemArchiveFile(const std::string& path);

        virtual Files::IStreamPtr open();

    private:
        std::string mPath;

    };

    class FileSystemArchive : public Archive
    {
    public:
        FileSystemArchive(const std::string& path);

        virtual void listResources(std::map<std::string, File*>& out, char (*normalize_function) (char));


    private:
        typedef std::map <std::string, FileSystemArchiveFile> index;
        index mIndex;

        bool mBuiltIndex;
        std::string mPath;

    };

}

#endif
