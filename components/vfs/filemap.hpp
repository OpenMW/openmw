#ifndef OPENMW_COMPONENTS_VFS_FILEMAP_H
#define OPENMW_COMPONENTS_VFS_FILEMAP_H

#include <map>
#include <string>

namespace VFS
{
    class File;

    using FileMap = std::map<std::string, File*, std::less<>>;
}

#endif
