#ifndef OPENMW_COMPONENTS_VFS_REGISTER_ARCHIVES_H
#define OPENMW_COMPONENTS_VFS_REGISTER_ARCHIVES_H

#include <components/files/collections.hpp>

namespace ToUTF8
{
    class StatelessUtf8Encoder;
}

namespace Loading
{
    class Listener;
}

namespace VFS
{
    class Manager;

    /// @brief Register BSA and file system archives based on the given OpenMW configuration.
    /// @param listener Gets the progress for every BSA archive and every file in the data directories.
    void registerArchives(VFS::Manager* vfs, const Files::Collections& collections,
        const std::vector<std::string>& archives, bool useLooseFiles, const ToUTF8::StatelessUtf8Encoder* encoder,
        Loading::Listener* listener = nullptr);
}

#endif
