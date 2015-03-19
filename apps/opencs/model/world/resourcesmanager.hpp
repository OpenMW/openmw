#ifndef CSM_WOLRD_RESOURCESMANAGER_H
#define CSM_WOLRD_RESOURCESMANAGER_H

#include <map>

#include "universalid.hpp"
#include "resources.hpp"

namespace VFS
{
    class Manager;
}

namespace CSMWorld
{
    class ResourcesManager
    {
            std::map<UniversalId::Type, Resources> mResources;

        private:

            void addResources (const Resources& resources);

        public:

            void setVFS(const VFS::Manager* vfs);

            const Resources& get (UniversalId::Type type) const;
    };
}

#endif
