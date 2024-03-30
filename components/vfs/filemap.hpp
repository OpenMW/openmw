#ifndef OPENMW_COMPONENTS_VFS_FILEMAP_H
#define OPENMW_COMPONENTS_VFS_FILEMAP_H

#include <map>
#include <string>

namespace VFS
{
    class File;

    namespace Path
    {
        class Normalized;
    }

    using FileMap = std::map<Path::Normalized, File*, std::less<>>;
}

#endif
