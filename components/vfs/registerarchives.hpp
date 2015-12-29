#ifndef OPENMW_COMPONENTS_VFS_REGISTER_ARCHIVES_H
#define OPENMW_COMPONENTS_VFS_REGISTER_ARCHIVES_H

#include <components/files/collections.hpp>

namespace VFS
{
    class Manager;

    /// @brief Register BSA and file system archives based on the given OpenMW configuration.
    void registerArchives (VFS::Manager* vfs, const Files::Collections& collections,
        const std::vector<std::string>& archives, bool useLooseFiles);
}

#endif
