#include "oscillatingrecastmeshobject.hpp"
#include "tilebounds.hpp"

#include <components/bullethelpers/aabb.hpp>

#include <algorithm>

namespace DetourNavigator
{
    namespace
    {
        void limitBy(btAABB& aabb, const TileBounds& bounds)
        {
            aabb.m_min.setX(std::max(aabb.m_min.x(), static_cast<btScalar>(bounds.mMin.x())));
            aabb.m_min.setY(std::max(aabb.m_min.y(), static_cast<btScalar>(bounds.mMin.y())));
            aabb.m_max.setX(std::min(aabb.m_max.x(), static_cast<btScalar>(bounds.mMax.x())));
            aabb.m_max.setY(std::min(aabb.m_max.y(), static_cast<btScalar>(bounds.mMax.y())));
        }
    }

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
                                             std::size_t lastChangeRevision, const TileBounds& bounds)
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
        limitBy(mAabb, bounds);
        return currentAabb != mAabb;
    }
}
