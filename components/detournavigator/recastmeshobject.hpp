#ifndef OPENMW_COMPONENTS_DETOURNAVIGATOR_RECASTMESHOBJECT_H
#define OPENMW_COMPONENTS_DETOURNAVIGATOR_RECASTMESHOBJECT_H

#include "areatype.hpp"

#include <components/resource/bulletshape.hpp>

#include <LinearMath/btTransform.h>

#include <osg/ref_ptr>

#include <functional>
#include <vector>

class btCollisionShape;
class btCompoundShape;

namespace DetourNavigator
{
    class CollisionShape
    {
    public:
        CollisionShape(osg::ref_ptr<const Resource::BulletShapeInstance> instance, const btCollisionShape& shape)
            : mShapeInstance(std::move(instance))
            , mShape(shape)
        {}

        const osg::ref_ptr<const Resource::BulletShapeInstance>& getShapeInstance() const { return mShapeInstance; }
        const btCollisionShape& getShape() const { return mShape; }

    private:
        osg::ref_ptr<const Resource::BulletShapeInstance> mShapeInstance;
        std::reference_wrapper<const btCollisionShape> mShape;
    };

    class RecastMeshObject
    {
        public:
            RecastMeshObject(const CollisionShape& shape, const btTransform& transform, const AreaType areaType);

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
            osg::ref_ptr<const Resource::BulletShapeInstance> mShapeInstance;
            std::reference_wrapper<const btCollisionShape> mShape;
            btTransform mTransform;
            AreaType mAreaType;
            btVector3 mLocalScaling;
            std::vector<RecastMeshObject> mChildren;
    };
}

#endif
