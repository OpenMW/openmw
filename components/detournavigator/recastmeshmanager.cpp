#include "recastmeshmanager.hpp"

#include <BulletCollision/CollisionShapes/btHeightfieldTerrainShape.h>

namespace DetourNavigator
{
    RecastMeshManager::RecastMeshManager(const Settings& settings, const TileBounds& bounds)
        : mShouldRebuild(false)
        , mMeshBuilder(settings, bounds)
    {
    }

    bool RecastMeshManager::addObject(std::size_t id, const btCollisionShape& shape, const btTransform& transform)
    {
        if (!mObjects.insert(std::make_pair(id, Object {&shape, transform})).second)
            return false;
        mShouldRebuild = true;
        return true;
    }

    boost::optional<RecastMeshManager::Object> RecastMeshManager::removeObject(std::size_t id)
    {
        const auto object = mObjects.find(id);
        if (object == mObjects.end())
            return boost::none;
        const auto result = object->second;
        mObjects.erase(object);
        mShouldRebuild = true;
        return result;
    }

    std::shared_ptr<RecastMesh> RecastMeshManager::getMesh()
    {
        rebuild();
        return mMeshBuilder.create();
    }

    bool RecastMeshManager::isEmpty() const
    {
        return mObjects.empty();
    }

    void RecastMeshManager::rebuild()
    {
        if (!mShouldRebuild)
            return;
        mMeshBuilder.reset();
        for (const auto& v : mObjects)
            mMeshBuilder.addObject(*v.second.mShape, v.second.mTransform);
        mShouldRebuild = false;
    }
}
