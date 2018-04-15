#ifndef OPENMW_COMPONENTS_DETOURNAVIGATOR_SHAREDNAVMESH_H
#define OPENMW_COMPONENTS_DETOURNAVIGATOR_SHAREDNAVMESH_H

#include <mutex>
#include <memory>

class dtNavMesh;

namespace DetourNavigator
{
    using NavMeshPtr = std::shared_ptr<dtNavMesh>;

    class LockedSharedNavMesh
    {
    public:
        LockedSharedNavMesh(std::mutex& mutex, const NavMeshPtr& value)
            : mLock(new std::lock_guard<std::mutex>(mutex)), mValue(value)
        {}

        dtNavMesh* operator ->() const
        {
            return mValue.get();
        }

        dtNavMesh& operator *() const
        {
            return *mValue;
        }

    private:
        std::unique_ptr<const std::lock_guard<std::mutex>> mLock;
        NavMeshPtr mValue;
    };

    class SharedNavMesh
    {
    public:
        SharedNavMesh(const NavMeshPtr& value)
            : mMutex(std::make_shared<std::mutex>()), mValue(value)
        {}

        LockedSharedNavMesh lock() const
        {
            return LockedSharedNavMesh(*mMutex, mValue);
        }

    private:
        std::shared_ptr<std::mutex> mMutex;
        NavMeshPtr mValue;
    };
}

#endif
