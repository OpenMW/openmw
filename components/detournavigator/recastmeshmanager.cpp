#include "recastmeshmanager.hpp"

#include <BulletCollision/CollisionShapes/btCompoundShape.h>
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
        if (!mObjects.emplace(id, RecastMeshObject(shape, transform)).second)
            return false;
        mShouldRebuild = true;
        return mShouldRebuild;
    }

    bool RecastMeshManager::updateObject(std::size_t id, const btTransform& transform)
    {
        const auto object = mObjects.find(id);
        if (object == mObjects.end())
            return false;
        if (!object->second.update(transform))
            return false;
        mShouldRebuild = true;
        return mShouldRebuild;
    }

    boost::optional<RemovedRecastMeshObject> RecastMeshManager::removeObject(std::size_t id)
    {
        const auto object = mObjects.find(id);
        if (object == mObjects.end())
            return boost::none;
        const RemovedRecastMeshObject result {object->second.getShape(), object->second.getTransform()};
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
            mMeshBuilder.addObject(v.second.getShape(), v.second.getTransform());
        mShouldRebuild = false;
    }
}
