#include "navmesh.hpp"

#include "vismask.hpp"

#include <components/detournavigator/guardednavmeshcacheitem.hpp>
#include <components/detournavigator/navmeshcacheitem.hpp>
#include <components/detournavigator/settings.hpp>
#include <components/resource/resourcesystem.hpp>
#include <components/resource/scenemanager.hpp>
#include <components/sceneutil/depth.hpp>
#include <components/sceneutil/detourdebugdraw.hpp>
#include <components/sceneutil/navmesh.hpp>
#include <components/sceneutil/workqueue.hpp>

#include <osg/BlendFunc>
#include <osg/LineWidth>
#include <osg/PositionAttitudeTransform>
#include <osg/StateSet>

#include <DetourNavMesh.h>

#include "../mwbase/environment.hpp"

#include <algorithm>
#include <limits>

namespace MWRender
{
    namespace
    {
        osg::ref_ptr<osg::StateSet> makeDebugDrawStateSet()
        {
            const osg::ref_ptr<osg::LineWidth> lineWidth = new osg::LineWidth();

            const osg::ref_ptr<osg::BlendFunc> blendFunc = new osg::BlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

            const osg::ref_ptr<SceneUtil::AutoDepth> depth = new SceneUtil::AutoDepth;
            depth->setWriteMask(false);

            osg::ref_ptr<osg::StateSet> stateSet = new osg::StateSet;
            stateSet->setMode(GL_BLEND, osg::StateAttribute::ON);
            stateSet->setMode(GL_LIGHTING, osg::StateAttribute::OFF);
            stateSet->setRenderingHint(osg::StateSet::TRANSPARENT_BIN);
            stateSet->setAttributeAndModes(lineWidth);
            stateSet->setAttributeAndModes(blendFunc);
            stateSet->setAttributeAndModes(depth);

            return stateSet;
        }
    }

    struct NavMesh::LessByTilePosition
    {
        bool operator()(const DetourNavigator::TilePosition& lhs,
            const std::pair<DetourNavigator::TilePosition, DetourNavigator::Version>& rhs) const
        {
            return lhs < rhs.first;
        }

        bool operator()(const std::pair<DetourNavigator::TilePosition, DetourNavigator::Version>& lhs,
            const DetourNavigator::TilePosition& rhs) const
        {
            return lhs.first < rhs;
        }
    };

    struct NavMesh::CreateNavMeshTileGroups final : SceneUtil::WorkItem
    {
        std::size_t mId;
        DetourNavigator::Version mVersion;
        const std::weak_ptr<DetourNavigator::GuardedNavMeshCacheItem> mNavMesh;
        const osg::ref_ptr<osg::StateSet> mGroupStateSet;
        const osg::ref_ptr<osg::StateSet> mDebugDrawStateSet;
        const DetourNavigator::Settings mSettings;
        std::map<DetourNavigator::TilePosition, Tile> mTiles;
        Settings::NavMeshRenderMode mMode;
        std::atomic_bool mAborted{ false };
        std::mutex mMutex;
        bool mStarted = false;
        std::vector<std::pair<DetourNavigator::TilePosition, Tile>> mUpdatedTiles;
        std::vector<DetourNavigator::TilePosition> mRemovedTiles;

        explicit CreateNavMeshTileGroups(std::size_t id, DetourNavigator::Version version,
            std::weak_ptr<DetourNavigator::GuardedNavMeshCacheItem> navMesh,
            const osg::ref_ptr<osg::StateSet>& groupStateSet, const osg::ref_ptr<osg::StateSet>& debugDrawStateSet,
            const DetourNavigator::Settings& settings, const std::map<DetourNavigator::TilePosition, Tile>& tiles,
            Settings::NavMeshRenderMode mode)
            : mId(id)
            , mVersion(version)
            , mNavMesh(std::move(navMesh))
            , mGroupStateSet(groupStateSet)
            , mDebugDrawStateSet(debugDrawStateSet)
            , mSettings(settings)
            , mTiles(tiles)
            , mMode(mode)
        {
        }

        void doWork() final
        {
            using DetourNavigator::TilePosition;
            using DetourNavigator::Version;

            const std::lock_guard lock(mMutex);
            mStarted = true;

            if (mAborted.load(std::memory_order_acquire))
                return;

            const auto navMeshPtr = mNavMesh.lock();
            if (navMeshPtr == nullptr)
                return;

            std::vector<std::pair<DetourNavigator::TilePosition, Version>> existingTiles;
            unsigned minSalt = std::numeric_limits<unsigned>::max();
            unsigned maxSalt = 0;

            navMeshPtr->lockConst()->forEachUsedTile(
                [&](const TilePosition& position, const Version& version, const dtMeshTile& meshTile) {
                    existingTiles.emplace_back(position, version);
                    minSalt = std::min(minSalt, meshTile.salt);
                    maxSalt = std::max(maxSalt, meshTile.salt);
                });

            if (mAborted.load(std::memory_order_acquire))
                return;

            std::sort(existingTiles.begin(), existingTiles.end());

            std::vector<DetourNavigator::TilePosition> removedTiles;

            for (const auto& [position, tile] : mTiles)
                if (!std::binary_search(existingTiles.begin(), existingTiles.end(), position, LessByTilePosition{}))
                    removedTiles.push_back(position);

            std::vector<std::pair<TilePosition, Tile>> updatedTiles;

            const unsigned char flags = SceneUtil::NavMeshTileDrawFlagsOffMeshConnections
                | SceneUtil::NavMeshTileDrawFlagsClosedList
                | (mMode == Settings::NavMeshRenderMode::UpdateFrequency ? SceneUtil::NavMeshTileDrawFlagsHeat : 0);

            for (const auto& [position, version] : existingTiles)
            {
                const auto it = mTiles.find(position);
                if (it != mTiles.end() && it->second.mGroup != nullptr && it->second.mVersion == version
                    && mMode != Settings::NavMeshRenderMode::UpdateFrequency)
                    continue;

                osg::ref_ptr<osg::Group> group;
                {
                    const auto navMesh = navMeshPtr->lockConst();
                    const dtMeshTile* meshTile = DetourNavigator::getTile(navMesh->getImpl(), position);
                    if (meshTile == nullptr)
                        continue;

                    if (mAborted.load(std::memory_order_acquire))
                        return;

                    group = SceneUtil::createNavMeshTileGroup(
                        navMesh->getImpl(), *meshTile, mSettings, mDebugDrawStateSet, flags, minSalt, maxSalt);
                }
                if (group == nullptr)
                {
                    removedTiles.push_back(position);
                    continue;
                }
                group->setNodeMask(Mask_Debug);
                group->setStateSet(mGroupStateSet);
                MWBase::Environment::get().getResourceSystem()->getSceneManager()->recreateShaders(group, "debug");
                updatedTiles.emplace_back(position, Tile{ version, std::move(group) });
            }

            if (mAborted.load(std::memory_order_acquire))
                return;

            mUpdatedTiles = std::move(updatedTiles);
            mRemovedTiles = std::move(removedTiles);
        }

        void abort() final { mAborted.store(true, std::memory_order_release); }
    };

    struct NavMesh::DeallocateCreateNavMeshTileGroups final : SceneUtil::WorkItem
    {
        osg::ref_ptr<NavMesh::CreateNavMeshTileGroups> mWorkItem;

        explicit DeallocateCreateNavMeshTileGroups(osg::ref_ptr<NavMesh::CreateNavMeshTileGroups>&& workItem)
            : mWorkItem(std::move(workItem))
        {
        }
    };

    NavMesh::NavMesh(const osg::ref_ptr<osg::Group>& root, const osg::ref_ptr<SceneUtil::WorkQueue>& workQueue,
        bool enabled, Settings::NavMeshRenderMode mode)
        : mRootNode(root)
        , mWorkQueue(workQueue)
        , mGroupStateSet(SceneUtil::makeDetourGroupStateSet())
        , mDebugDrawStateSet(makeDebugDrawStateSet())
        , mEnabled(enabled)
        , mMode(mode)
        , mId(std::numeric_limits<std::size_t>::max())
    {
    }

    NavMesh::~NavMesh()
    {
        if (mEnabled)
            disable();
        for (const auto& workItem : mWorkItems)
            workItem->abort();
    }

    bool NavMesh::toggle()
    {
        if (mEnabled)
            disable();
        else
            enable();

        return mEnabled;
    }

    void NavMesh::update(const std::shared_ptr<DetourNavigator::GuardedNavMeshCacheItem>& navMesh, std::size_t id,
        const DetourNavigator::Settings& settings)
    {
        using DetourNavigator::TilePosition;
        using DetourNavigator::Version;

        if (!mEnabled)
            return;

        {
            std::pair<std::size_t, Version> lastest{ 0, Version{} };
            osg::ref_ptr<CreateNavMeshTileGroups> latestCandidate;
            for (auto it = mWorkItems.begin(); it != mWorkItems.end();)
            {
                if (!(*it)->isDone())
                {
                    ++it;
                    continue;
                }
                const std::pair<std::size_t, Version> order{ (*it)->mId, (*it)->mVersion };
                if (lastest < order)
                {
                    lastest = order;
                    std::swap(latestCandidate, *it);
                }
                if (*it != nullptr)
                    mWorkQueue->addWorkItem(new DeallocateCreateNavMeshTileGroups(std::move(*it)));
                it = mWorkItems.erase(it);
            }

            if (latestCandidate != nullptr)
            {
                for (const TilePosition& position : latestCandidate->mRemovedTiles)
                {
                    const auto it = mTiles.find(position);
                    if (it == mTiles.end())
                        continue;
                    mRootNode->removeChild(it->second.mGroup);
                    mTiles.erase(it);
                }

                for (auto& [position, tile] : latestCandidate->mUpdatedTiles)
                {
                    const auto it = mTiles.find(position);
                    if (it == mTiles.end())
                    {
                        mRootNode->addChild(tile.mGroup);
                        mTiles.emplace_hint(it, position, std::move(tile));
                    }
                    else
                    {
                        mRootNode->replaceChild(it->second.mGroup, tile.mGroup);
                        std::swap(it->second, tile);
                    }
                }

                mWorkQueue->addWorkItem(new DeallocateCreateNavMeshTileGroups(std::move(latestCandidate)));
            }
        }

        const auto version = navMesh->lock()->getVersion();

        if (!mTiles.empty() && mId == id && mVersion == version)
            return;

        if (mId != id)
        {
            reset();
            mId = id;
        }

        mVersion = version;

        for (auto& workItem : mWorkItems)
        {
            const std::unique_lock lock(workItem->mMutex, std::try_to_lock);

            if (!lock.owns_lock())
                continue;

            if (workItem->mStarted)
                continue;

            workItem->mId = id;
            workItem->mVersion = version;
            workItem->mTiles = mTiles;
            workItem->mMode = mMode;

            return;
        }

        osg::ref_ptr<CreateNavMeshTileGroups> workItem = new CreateNavMeshTileGroups(
            id, version, navMesh, mGroupStateSet, mDebugDrawStateSet, settings, mTiles, mMode);
        mWorkQueue->addWorkItem(workItem);
        mWorkItems.push_back(std::move(workItem));
    }

    void NavMesh::reset()
    {
        for (auto& workItem : mWorkItems)
            workItem->abort();
        mWorkItems.clear();
        for (auto& [position, tile] : mTiles)
            mRootNode->removeChild(tile.mGroup);
        mTiles.clear();
    }

    void NavMesh::enable()
    {
        mEnabled = true;
    }

    void NavMesh::disable()
    {
        reset();
        mEnabled = false;
    }

    void NavMesh::setMode(Settings::NavMeshRenderMode value)
    {
        if (mMode == value)
            return;
        reset();
        mMode = value;
    }
}
