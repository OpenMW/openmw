#ifndef OPENMW_COMPONENTS_VFS_FILE_H
#define OPENMW_COMPONENTS_VFS_FILE_H

#include <filesystem>
#include <string>

#include <components/files/istreamptr.hpp>

namespace VFS
{
    class File
    {
    public:
        virtual ~File() = default;

        virtual Files::IStreamPtr open() = 0;

        virtual std::filesystem::file_time_type getLastModified() const = 0;

        virtual std::string getStem() const = 0;
    };
}

#endif
