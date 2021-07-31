#include "recastmeshobject.hpp"

#include <components/debug/debuglog.hpp>

#include <BulletCollision/CollisionShapes/btCompoundShape.h>

#include <cassert>

namespace DetourNavigator
{
    namespace
    {
        bool updateCompoundObject(const btCompoundShape& shape, const AreaType areaType,
            std::vector<RecastMeshObject>& children)
        {
            assert(static_cast<std::size_t>(shape.getNumChildShapes()) == children.size());
            bool result = false;
            for (int i = 0, num = shape.getNumChildShapes(); i < num; ++i)
            {
                assert(shape.getChildShape(i) == std::addressof(children[static_cast<std::size_t>(i)].getShape()));
                result = children[static_cast<std::size_t>(i)].update(shape.getChildTransform(i), areaType) || result;
            }
            return result;
        }

        std::vector<RecastMeshObject> makeChildrenObjects(const osg::ref_ptr<const osg::Object>& holder,
                                                          const btCompoundShape& shape, const AreaType areaType)
        {
            std::vector<RecastMeshObject> result;
            for (int i = 0, num = shape.getNumChildShapes(); i < num; ++i)
            {
                const CollisionShape collisionShape {holder, *shape.getChildShape(i)};
                result.emplace_back(collisionShape, shape.getChildTransform(i), areaType);
            }
            return result;
        }

        std::vector<RecastMeshObject> makeChildrenObjects(const osg::ref_ptr<const osg::Object>& holder,
                                                          const btCollisionShape& shape, const AreaType areaType)
        {
            if (shape.isCompound())
                return makeChildrenObjects(holder, static_cast<const btCompoundShape&>(shape), areaType);
            return std::vector<RecastMeshObject>();
        }
    }

    RecastMeshObject::RecastMeshObject(const CollisionShape& shape, const btTransform& transform,
            const AreaType areaType)
        : mHolder(shape.getHolder())
        , mShape(shape.getShape())
        , mTransform(transform)
        , mAreaType(areaType)
        , mLocalScaling(mShape.get().getLocalScaling())
        , mChildren(makeChildrenObjects(mHolder, mShape.get(), mAreaType))
    {
    }

    bool RecastMeshObject::update(const btTransform& transform, const AreaType areaType)
    {
        bool result = false;
        if (!(mTransform == transform))
        {
            mTransform = transform;
            result = true;
        }
        if (mAreaType != areaType)
        {
            mAreaType = areaType;
            result = true;
        }
        if (!(mLocalScaling == mShape.get().getLocalScaling()))
        {
            mLocalScaling = mShape.get().getLocalScaling();
            result = true;
        }
        if (mShape.get().isCompound())
            result = updateCompoundObject(static_cast<const btCompoundShape&>(mShape.get()), mAreaType, mChildren)
                    || result;
        return result;
    }
}
