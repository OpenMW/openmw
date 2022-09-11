#include "commulativeaabb.hpp"

#include <components/bullethelpers/aabb.hpp>

namespace DetourNavigator
{
    CommulativeAabb::CommulativeAabb(std::size_t lastChangeRevision, const btAABB& aabb)
        : mLastChangeRevision(lastChangeRevision)
        , mAabb(aabb)
    {
    }

    bool CommulativeAabb::update(std::size_t lastChangeRevision, const btAABB& aabb)
    {
        if (mLastChangeRevision != lastChangeRevision)
        {
            mLastChangeRevision = lastChangeRevision;
            // btAABB doesn't have copy-assignment operator
            mAabb.m_min = aabb.m_min;
            mAabb.m_max = aabb.m_max;
            return true;
        }
        const btAABB currentAabb = mAabb;
        mAabb.merge(aabb);
        return currentAabb != mAabb;
    }
}
