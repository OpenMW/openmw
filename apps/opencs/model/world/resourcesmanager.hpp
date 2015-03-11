#ifndef CSM_WOLRD_RESOURCESMANAGER_H
#define CSM_WOLRD_RESOURCESMANAGER_H

#include <map>

#include "universalid.hpp"
#include "resources.hpp"

namespace CSMWorld
{
    class ResourcesManager
    {
            std::map<UniversalId::Type, Resources> mResources;

        private:

            void addResources (const Resources& resources);

        public:

            /// Ask OGRE for a list of available resources.
            void listResources();

            const Resources& get (UniversalId::Type type) const;
    };
}

#endif
