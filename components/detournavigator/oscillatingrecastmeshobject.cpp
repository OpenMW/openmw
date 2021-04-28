#include "oscillatingrecastmeshobject.hpp"

#include <components/bullethelpers/aabb.hpp>

namespace DetourNavigator
{
    OscillatingRecastMeshObject::OscillatingRecastMeshObject(RecastMeshObject&& impl, std::size_t lastChangeRevision)
        : mImpl(std::move(impl))
        , mLastChangeRevision(lastChangeRevision)
        , mAabb(BulletHelpers::getAabb(mImpl.getShape(), mImpl.getTransform()))
    {
    }

    OscillatingRecastMeshObject::OscillatingRecastMeshObject(const RecastMeshObject& impl, std::size_t lastChangeRevision)
        : mImpl(impl)
        , mLastChangeRevision(lastChangeRevision)
        , mAabb(BulletHelpers::getAabb(mImpl.getShape(), mImpl.getTransform()))
    {
    }

    bool OscillatingRecastMeshObject::update(const btTransform& transform, const AreaType areaType,
                                             std::size_t lastChangeRevision)
    {
        const btTransform oldTransform = mImpl.getTransform();
        if (!mImpl.update(transform, areaType))
            return false;
        if (transform == oldTransform)
            return true;
        if (mLastChangeRevision != lastChangeRevision)
        {
            mLastChangeRevision = lastChangeRevision;
            // btAABB doesn't have copy-assignment operator
            const btAABB aabb = BulletHelpers::getAabb(mImpl.getShape(), transform);
            mAabb.m_min = aabb.m_min;
            mAabb.m_max = aabb.m_max;
            return true;
        }
        const btAABB currentAabb = mAabb;
        mAabb.merge(BulletHelpers::getAabb(mImpl.getShape(), transform));
        return currentAabb != mAabb;
    }
}
