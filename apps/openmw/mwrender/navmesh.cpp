#include "navmesh.hpp"
#include "vismask.hpp"

#include <components/sceneutil/navmesh.hpp>
#include <components/resource/resourcesystem.hpp>
#include <components/resource/scenemanager.hpp>
#include <components/detournavigator/navmeshcacheitem.hpp>
#include <components/sceneutil/detourdebugdraw.hpp>

#include <osg/PositionAttitudeTransform>
#include <osg/StateSet>

#include "../mwbase/world.hpp"
#include "../mwbase/environment.hpp"

#include <limits>

namespace MWRender
{
    NavMesh::NavMesh(const osg::ref_ptr<osg::Group>& root, bool enabled)
        : mRootNode(root)
        , mGroupStateSet(SceneUtil::makeNavMeshTileStateSet())
        , mDebugDrawStateSet(SceneUtil::DebugDraw::makeStateSet())
        , mEnabled(enabled)
        , mId(std::numeric_limits<std::size_t>::max())
    {
    }

    NavMesh::~NavMesh()
    {
        if (mEnabled)
            disable();
    }

    bool NavMesh::toggle()
    {
        if (mEnabled)
            disable();
        else
            enable();

        return mEnabled;
    }

    void NavMesh::update(const DetourNavigator::NavMeshCacheItem& navMesh, std::size_t id,
        const DetourNavigator::Settings& settings)
    {
        using DetourNavigator::TilePosition;
        using DetourNavigator::Version;

        if (!mEnabled || (!mTiles.empty() && mId == id && mVersion == navMesh.getVersion()))
            return;

        if (mId != id)
        {
            reset();
            mId = id;
        }

        mVersion = navMesh.getVersion();

        std::vector<TilePosition> updated;
        navMesh.forEachUsedTile([&] (const TilePosition& position, const Version& version, const dtMeshTile& meshTile)
        {
            updated.push_back(position);
            Tile& tile = mTiles[position];
            if (tile.mGroup != nullptr && tile.mVersion == version)
                return;
            if (tile.mGroup != nullptr)
                mRootNode->removeChild(tile.mGroup);
            tile.mGroup = SceneUtil::createNavMeshTileGroup(navMesh.getImpl(), meshTile, settings,
                                                            mGroupStateSet, mDebugDrawStateSet);
            if (tile.mGroup == nullptr)
                return;
            MWBase::Environment::get().getResourceSystem()->getSceneManager()->recreateShaders(tile.mGroup, "debug");
            tile.mGroup->setNodeMask(Mask_Debug);
            mRootNode->addChild(tile.mGroup);
        });
        std::sort(updated.begin(), updated.end());
        for (auto it = mTiles.begin(); it != mTiles.end();)
        {
            if (!std::binary_search(updated.begin(), updated.end(), it->first))
            {
                mRootNode->removeChild(it->second.mGroup);
                it = mTiles.erase(it);
            }
            else
                ++it;
        }
    }

    void NavMesh::reset()
    {
        for (auto& [position, tile] : mTiles)
            mRootNode->removeChild(tile.mGroup);
        mTiles.clear();
    }

    void NavMesh::enable()
    {
        for (const auto& [position, tile] : mTiles)
            mRootNode->addChild(tile.mGroup);
        mEnabled = true;
    }

    void NavMesh::disable()
    {
        for (const auto& [position, tile] : mTiles)
            mRootNode->removeChild(tile.mGroup);
        mEnabled = false;
    }
}
