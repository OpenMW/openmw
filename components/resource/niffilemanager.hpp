#ifndef OPENMW_COMPONENTS_RESOURCE_NIFFILEMANAGER_H
#define OPENMW_COMPONENTS_RESOURCE_NIFFILEMANAGER_H

#include <components/nif/niffile.hpp>

#include "resourcemanager.hpp"

namespace ToUTF8
{
    class StatelessUtf8Encoder;
}

namespace Resource
{

    /// @brief Handles caching of NIFFiles.
    /// @note May be used from any thread.
    class NifFileManager : public ResourceManager
    {
        const ToUTF8::StatelessUtf8Encoder* mEncoder;

    public:
        NifFileManager(const VFS::Manager* vfs, const ToUTF8::StatelessUtf8Encoder* encoder);
        ~NifFileManager();

        /// Retrieve a NIF file from the cache, or load it from the VFS if not cached yet.
        /// @note For performance reasons the NifFileManager does not handle case folding, needs
        /// to be done in advance by other managers accessing the NifFileManager.
        Nif::NIFFilePtr get(VFS::Path::NormalizedView name);

        void reportStats(unsigned int frameNumber, osg::Stats* stats) const override;
    };

}

#endif
