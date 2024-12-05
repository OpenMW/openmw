#ifndef OPENMW_COMPONENTS_VFS_ARCHIVE_H
#define OPENMW_COMPONENTS_VFS_ARCHIVE_H

#include <string>

#include "filemap.hpp"
#include "pathutil.hpp"

namespace VFS
{
    class Archive
    {
    public:
        virtual ~Archive() = default;

        /// List all resources contained in this archive.
        virtual void listResources(FileMap& out) = 0;

        /// True if this archive contains the provided normalized file.
        virtual bool contains(Path::NormalizedView file) const = 0;

        virtual std::string getDescription() const = 0;
    };

}

#endif
