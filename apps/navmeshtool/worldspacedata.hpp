#ifndef OPENMW_NAVMESHTOOL_WORLDSPACEDATA_H
#define OPENMW_NAVMESHTOOL_WORLDSPACEDATA_H

#include <components/bullethelpers/collisionobject.hpp>
#include <components/detournavigator/tilecachedrecastmeshmanager.hpp>
#include <components/esm3/loadland.hpp>
#include <components/misc/convert.hpp>
#include <components/resource/bulletshape.hpp>

#include <BulletCollision/CollisionDispatch/btCollisionObject.h>
#include <BulletCollision/Gimpact/btBoxCollision.h>
#include <LinearMath/btVector3.h>

#include <memory>
#include <regex>
#include <span>
#include <vector>

namespace ESM
{
    class ESMReader;
    class ReadersCache;
}

namespace VFS
{
    class Manager;
}

namespace Resource
{
    class BulletShapeManager;
}

namespace EsmLoader
{
    struct EsmData;
}

namespace DetourNavigator
{
    struct Settings;
}

namespace NavMeshTool
{
    using DetourNavigator::ObjectTransform;
    using DetourNavigator::TileCachedRecastMeshManager;

    class BulletObject
    {
    public:
        BulletObject(osg::ref_ptr<Resource::BulletShapeInstance>&& shapeInstance, const ESM::Position& position,
            float localScaling)
            : mShapeInstance(std::move(shapeInstance))
            , mObjectTransform{ position, localScaling }
            , mCollisionObject(BulletHelpers::makeCollisionObject(mShapeInstance->mCollisionShape.get(),
                  Misc::Convert::toBullet(position.asVec3()),
                  Misc::Convert::toBullet(Misc::Convert::makeOsgQuat(position))))
        {
            mShapeInstance->setLocalScaling(btVector3(localScaling, localScaling, localScaling));
        }

        const osg::ref_ptr<Resource::BulletShapeInstance>& getShapeInstance() const noexcept { return mShapeInstance; }
        const DetourNavigator::ObjectTransform& getObjectTransform() const noexcept { return mObjectTransform; }
        btCollisionObject& getCollisionObject() const noexcept { return *mCollisionObject; }

    private:
        osg::ref_ptr<Resource::BulletShapeInstance> mShapeInstance;
        DetourNavigator::ObjectTransform mObjectTransform;
        std::unique_ptr<btCollisionObject> mCollisionObject;
    };

    struct WorldspaceData
    {
        ESM::RefId mWorldspace;
        std::unique_ptr<TileCachedRecastMeshManager> mTileCachedRecastMeshManager;
        btAABB mAabb;
        bool mAabbInitialized = false;
        std::vector<BulletObject> mObjects;
        std::vector<std::unique_ptr<ESM::Land::LandData>> mLandData;
        std::vector<std::vector<float>> mHeightfields;

        WorldspaceData(ESM::RefId worldspace, const DetourNavigator::RecastSettings& settings);
    };

    std::unordered_map<ESM::RefId, std::vector<std::size_t>> collectWorldspaceCells(
        const EsmLoader::EsmData& esmData, bool processInteriorCells, const std::regex& worldspaceFilter);

    WorldspaceData gatherWorldspaceData(const DetourNavigator::Settings& settings, ESM::ReadersCache& readers,
        const VFS::Manager& vfs, Resource::BulletShapeManager& bulletShapeManager, const EsmLoader::EsmData& esmData,
        bool writeBinaryLog, ESM::RefId worldspace, std::span<const std::size_t> cells);
}

#endif
