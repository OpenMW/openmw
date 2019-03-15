#ifndef OPENMW_COMPONENTS_DETOURNAVIGATOR_RECASTMESHOBJECT_H
#define OPENMW_COMPONENTS_DETOURNAVIGATOR_RECASTMESHOBJECT_H

#include "areatype.hpp"

#include <LinearMath/btTransform.h>

#include <functional>
#include <vector>

class btCollisionShape;
class btCompoundShape;

namespace DetourNavigator
{
    class RecastMeshObject
    {
        public:
            RecastMeshObject(const btCollisionShape& shape, const btTransform& transform, const AreaType areaType);

            bool update(const btTransform& transform, const AreaType areaType);

            const btCollisionShape& getShape() const
            {
                return mShape;
            }

            const btTransform& getTransform() const
            {
                return mTransform;
            }

            AreaType getAreaType() const
            {
                return mAreaType;
            }

        private:
            std::reference_wrapper<const btCollisionShape> mShape;
            btTransform mTransform;
            AreaType mAreaType;
            btVector3 mLocalScaling;
            std::vector<RecastMeshObject> mChildren;

            static bool updateCompoundObject(const btCompoundShape& shape, const AreaType areaType,
                std::vector<RecastMeshObject>& children);
    };

    std::vector<RecastMeshObject> makeChildrenObjects(const btCollisionShape& shape, const AreaType areaType);

    std::vector<RecastMeshObject> makeChildrenObjects(const btCompoundShape& shape, const AreaType areaType);
}

#endif
