#ifndef OPENMW_COMPONENTS_DETOURNAVIGATOR_RECASTMESHOBJECT_H
#define OPENMW_COMPONENTS_DETOURNAVIGATOR_RECASTMESHOBJECT_H

#include "areatype.hpp"
#include "objecttransform.hpp"

#include <components/resource/bulletshape.hpp>

#include <LinearMath/btTransform.h>

#include <osg/ref_ptr>
#include <osg/Referenced>

#include <functional>
#include <vector>

class btCollisionShape;
class btCompoundShape;

namespace DetourNavigator
{
    class CollisionShape
    {
    public:
        CollisionShape(osg::ref_ptr<const Resource::BulletShapeInstance> instance, const btCollisionShape& shape,
                       const ObjectTransform& transform)
            : mInstance(std::move(instance))
            , mShape(shape)
            , mObjectTransform(transform)
        {}

        const osg::ref_ptr<const Resource::BulletShapeInstance>& getInstance() const { return mInstance; }
        const btCollisionShape& getShape() const { return mShape; }
        const ObjectTransform& getObjectTransform() const { return mObjectTransform; }

    private:
        osg::ref_ptr<const Resource::BulletShapeInstance> mInstance;
        std::reference_wrapper<const btCollisionShape> mShape;
        ObjectTransform mObjectTransform;
    };

    class ChildRecastMeshObject
    {
        public:
            ChildRecastMeshObject(const btCollisionShape& shape, const btTransform& transform, const AreaType areaType);

            bool update(const btTransform& transform, const AreaType areaType);

            const btCollisionShape& getShape() const { return mShape; }

            const btTransform& getTransform() const { return mTransform; }

            AreaType getAreaType() const { return mAreaType; }

        private:
            std::reference_wrapper<const btCollisionShape> mShape;
            btTransform mTransform;
            AreaType mAreaType;
            btVector3 mLocalScaling;
            std::vector<ChildRecastMeshObject> mChildren;
    };

    class RecastMeshObject
    {
        public:
            RecastMeshObject(const CollisionShape& shape, const btTransform& transform, const AreaType areaType);

            bool update(const btTransform& transform, const AreaType areaType) { return mImpl.update(transform, areaType); }

            const osg::ref_ptr<const Resource::BulletShapeInstance>& getInstance() const { return mInstance; }

            const btCollisionShape& getShape() const { return mImpl.getShape(); }

            const btTransform& getTransform() const { return mImpl.getTransform(); }

            AreaType getAreaType() const { return mImpl.getAreaType(); }

            const ObjectTransform& getObjectTransform() const { return mObjectTransform; }

        private:
            osg::ref_ptr<const Resource::BulletShapeInstance> mInstance;
            ObjectTransform mObjectTransform;
            ChildRecastMeshObject mImpl;
    };
}

#endif
