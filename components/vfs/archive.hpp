#ifndef OPENMW_COMPONENTS_RESOURCE_ARCHIVE_H
#define OPENMW_COMPONENTS_RESOURCE_ARCHIVE_H

#include <map>

#include <components/files/constrainedfilestream.hpp>

namespace VFS
{

    class File
    {
    public:
        virtual ~File() {}

        virtual Files::IStreamPtr open() = 0;
    };

    class Archive
    {
    public:
        virtual ~Archive() {}

        /// List all resources contained in this archive, and run the resource names through the given normalize function.
        virtual void listResources(std::map<std::string, File*>& out, char (*normalize_function) (char)) = 0;

        /// True if this archive contains the provided normalized file.
        virtual bool contains(const std::string& file, char (*normalize_function) (char)) const = 0;

        virtual std::string getDescription() const = 0;
    };

}

#endif
