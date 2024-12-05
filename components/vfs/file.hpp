#ifndef OPENMW_COMPONENTS_VFS_FILE_H
#define OPENMW_COMPONENTS_VFS_FILE_H

#include <filesystem>

#include <components/files/istreamptr.hpp>

namespace VFS
{
    class File
    {
    public:
        virtual ~File() = default;

        virtual Files::IStreamPtr open() = 0;

        virtual std::filesystem::path getPath() = 0;
    };
}

#endif
