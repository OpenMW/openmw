#include "recastmeshobject.hpp"

#include <components/debug/debuglog.hpp>

#include <BulletCollision/CollisionShapes/btCompoundShape.h>

#include <numeric>

namespace DetourNavigator
{
    RecastMeshObject::RecastMeshObject(const btCollisionShape& shape, const btTransform& transform,
            const unsigned char flags)
        : mShape(shape)
        , mTransform(transform)
        , mFlags(flags)
        , mChildren(makeChildrenObjects(shape, mFlags))
    {
    }

    bool RecastMeshObject::update(const btTransform& transform, const unsigned char flags)
    {
        bool result = false;
        if (!(mTransform == transform))
        {
            mTransform = transform;
            result = true;
        }
        if (mFlags != flags)
        {
            mFlags = flags;
            result = true;
        }
        if (mShape.get().isCompound())
            result = updateCompoundObject(static_cast<const btCompoundShape&>(mShape.get()), mFlags, mChildren) || result;
        return result;
    }

    bool RecastMeshObject::updateCompoundObject(const btCompoundShape& shape,
        const unsigned char flags, std::vector<RecastMeshObject>& children)
    {
        assert(static_cast<std::size_t>(shape.getNumChildShapes()) == children.size());
        bool result = false;
        for (int i = 0, num = shape.getNumChildShapes(); i < num; ++i)
        {
            assert(shape.getChildShape(i) == std::addressof(children[static_cast<std::size_t>(i)].mShape.get()));
            result = children[static_cast<std::size_t>(i)].update(shape.getChildTransform(i), flags) || result;
        }
        return result;
    }

    std::vector<RecastMeshObject> makeChildrenObjects(const btCollisionShape& shape, const unsigned char flags)
    {
        if (shape.isCompound())
            return makeChildrenObjects(static_cast<const btCompoundShape&>(shape), flags);
        else
            return std::vector<RecastMeshObject>();
    }

    std::vector<RecastMeshObject> makeChildrenObjects(const btCompoundShape& shape, const unsigned char flags)
    {
        std::vector<RecastMeshObject> result;
        for (int i = 0, num = shape.getNumChildShapes(); i < num; ++i)
            result.emplace_back(*shape.getChildShape(i), shape.getChildTransform(i), flags);
        return result;
    }
}
