#ifndef OPENMW_COMPONENTS_DETOURNAVIGATOR_OSCILLATINGRECASTMESHOBJECT_H
#define OPENMW_COMPONENTS_DETOURNAVIGATOR_OSCILLATINGRECASTMESHOBJECT_H

#include "areatype.hpp"
#include "recastmeshobject.hpp"

#include <LinearMath/btTransform.h>
#include <BulletCollision/Gimpact/btBoxCollision.h>

namespace DetourNavigator
{
    class OscillatingRecastMeshObject
    {
        public:
            explicit OscillatingRecastMeshObject(RecastMeshObject impl, std::size_t lastChangeRevision);

            bool update(const btTransform& transform, const AreaType areaType, std::size_t lastChangeRevision);

            const RecastMeshObject& getImpl() const { return mImpl; }

        private:
            RecastMeshObject mImpl;
            std::size_t mLastChangeRevision;
            btAABB mAabb;
    };
}

#endif
