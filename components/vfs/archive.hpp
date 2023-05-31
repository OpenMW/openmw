#ifndef OPENMW_COMPONENTS_RESOURCE_ARCHIVE_H
#define OPENMW_COMPONENTS_RESOURCE_ARCHIVE_H

#include <filesystem>
#include <map>

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

    class Archive
    {
    public:
        virtual ~Archive() = default;

        /// List all resources contained in this archive.
        virtual void listResources(std::map<std::string, File*>& out) = 0;

        /// True if this archive contains the provided normalized file.
        virtual bool contains(const std::string& file) const = 0;

        virtual std::string getDescription() const = 0;
    };

}

#endif
