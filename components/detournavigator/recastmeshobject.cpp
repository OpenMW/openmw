#include "recastmeshobject.hpp"

#include <components/debug/debuglog.hpp>

#include <BulletCollision/CollisionShapes/btCompoundShape.h>

#include <cassert>

namespace DetourNavigator
{
    namespace
    {
        bool updateCompoundObject(const btCompoundShape& shape, const AreaType areaType,
            std::vector<ChildRecastMeshObject>& children)
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

        std::vector<ChildRecastMeshObject> makeChildrenObjects(const btCompoundShape& shape, const AreaType areaType)
        {
            std::vector<ChildRecastMeshObject> result;
            for (int i = 0, num = shape.getNumChildShapes(); i < num; ++i)
                result.emplace_back(*shape.getChildShape(i), shape.getChildTransform(i), areaType);
            return result;
        }

        std::vector<ChildRecastMeshObject> makeChildrenObjects(const btCollisionShape& shape, const AreaType areaType)
        {
            if (shape.isCompound())
                return makeChildrenObjects(static_cast<const btCompoundShape&>(shape), areaType);
            return {};
        }
    }

    ChildRecastMeshObject::ChildRecastMeshObject(const btCollisionShape& shape, const btTransform& transform,
            const AreaType areaType)
        : mShape(shape)
        , mTransform(transform)
        , mAreaType(areaType)
        , mLocalScaling(shape.getLocalScaling())
        , mChildren(makeChildrenObjects(shape, mAreaType))
    {
    }

    bool ChildRecastMeshObject::update(const btTransform& transform, const AreaType areaType)
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

    RecastMeshObject::RecastMeshObject(const CollisionShape& shape, const btTransform& transform,
            const AreaType areaType)
        : mHolder(shape.getHolder())
        , mImpl(shape.getShape(), transform, areaType)
    {
    }
}
