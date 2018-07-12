#ifndef OPENMW_COMPONENTS_DETOURNAVIGATOR_RECASTMESHOBJECT_H
#define OPENMW_COMPONENTS_DETOURNAVIGATOR_RECASTMESHOBJECT_H

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
            RecastMeshObject(const btCollisionShape& shape, const btTransform& transform, const unsigned char flags);

            bool update(const btTransform& transform, const unsigned char flags);

            const btCollisionShape& getShape() const
            {
                return mShape;
            }

            const btTransform& getTransform() const
            {
                return mTransform;
            }

            unsigned char getFlags() const
            {
                return mFlags;
            }

        private:
            std::reference_wrapper<const btCollisionShape> mShape;
            btTransform mTransform;
            unsigned char mFlags;
            std::vector<RecastMeshObject> mChildren;

            static bool updateCompoundObject(const btCompoundShape& shape, const unsigned char flags,
                std::vector<RecastMeshObject>& children);
    };

    std::vector<RecastMeshObject> makeChildrenObjects(const btCollisionShape& shape, const unsigned char flags);

    std::vector<RecastMeshObject> makeChildrenObjects(const btCompoundShape& shape, const unsigned char flags);
}

#endif
