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
            RecastMeshObject(const btCollisionShape& shape, const btTransform& transform);

            bool update(const btTransform& transform);

            const btCollisionShape& getShape() const
            {
                return mShape;
            }

            const btTransform& getTransform() const
            {
                return mTransform;
            }

        private:
            std::reference_wrapper<const btCollisionShape> mShape;
            btTransform mTransform;
            std::vector<RecastMeshObject> mChildren;

            static bool updateCompoundObject(const btCompoundShape& shape, std::vector<RecastMeshObject>& children);
    };

    std::vector<RecastMeshObject> makeChildrenObjects(const btCollisionShape& shape);

    std::vector<RecastMeshObject> makeChildrenObjects(const btCompoundShape& shape);
}

#endif
