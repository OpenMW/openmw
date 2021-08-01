#ifndef OPENMW_COMPONENTS_DETOURNAVIGATOR_RECASTMESHOBJECT_H
#define OPENMW_COMPONENTS_DETOURNAVIGATOR_RECASTMESHOBJECT_H

#include "areatype.hpp"

#include <LinearMath/btTransform.h>

#include <osg/ref_ptr>
#include <osg/Object>

#include <functional>
#include <vector>

class btCollisionShape;
class btCompoundShape;

namespace DetourNavigator
{
    class CollisionShape
    {
    public:
        CollisionShape(osg::ref_ptr<const osg::Object> holder, const btCollisionShape& shape)
            : mHolder(std::move(holder))
            , mShape(shape)
        {}

        const osg::ref_ptr<const osg::Object>& getHolder() const { return mHolder; }
        const btCollisionShape& getShape() const { return mShape; }

    private:
        osg::ref_ptr<const osg::Object> mHolder;
        std::reference_wrapper<const btCollisionShape> mShape;
    };

    class RecastMeshObject
    {
        public:
            RecastMeshObject(const CollisionShape& shape, const btTransform& transform, const AreaType areaType);

            bool update(const btTransform& transform, const AreaType areaType);

            const osg::ref_ptr<const osg::Object>& getHolder() const
            {
                return mHolder;
            }

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
            osg::ref_ptr<const osg::Object> mHolder;
            std::reference_wrapper<const btCollisionShape> mShape;
            btTransform mTransform;
            AreaType mAreaType;
            btVector3 mLocalScaling;
            std::vector<RecastMeshObject> mChildren;
    };
}

#endif
