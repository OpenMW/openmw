#ifndef OPENMW_MWWORLD_CELLPRELOADER_H
#define OPENMW_MWWORLD_CELLPRELOADER_H

#include "positioncellgrid.hpp"

#include <components/sceneutil/workqueue.hpp>

#include <osg/ref_ptr>

#include <map>

namespace osg
{
    class Stats;
}

namespace Resource
{
    class ResourceSystem;
    class BulletShapeManager;
}

namespace Terrain
{
    class World;
    class View;
}

namespace MWRender
{
    class LandManager;
}

namespace Loading
{
    class Listener;
}

namespace MWWorld
{
    class CellStore;
    class TerrainPreloadItem;

    class CellPreloader
    {
    public:
        CellPreloader(Resource::ResourceSystem* resourceSystem, Resource::BulletShapeManager* bulletShapeManager,
            Terrain::World* terrain, MWRender::LandManager* landManager);
        ~CellPreloader();

        /// Ask a background thread to preload rendering meshes and collision shapes for objects in this cell.
        /// @note The cell itself must be in State_Loaded or State_Preloaded.
        void preload(MWWorld::CellStore& cell, double timestamp);

        void notifyLoaded(MWWorld::CellStore* cell);

        void clear();

        /// Removes preloaded cells that have not had a preload request for a while.
        void updateCache(double timestamp);

        /// How long to keep a preloaded cell in cache after it's no longer requested.
        void setExpiryDelay(double expiryDelay);

        /// The minimum number of preloaded cells before unused cells get thrown out.
        void setMinCacheSize(std::size_t value) { mMinCacheSize = value; }

        /// The maximum number of preloaded cells.
        void setMaxCacheSize(std::size_t value) { mMaxCacheSize = value; }

        /// Enables the creation of instances in the preloading thread.
        void setPreloadInstances(bool preload);

        std::size_t getMaxCacheSize() const { return mMaxCacheSize; }

        std::size_t getCacheSize() const { return mPreloadCells.size(); }

        void setWorkQueue(osg::ref_ptr<SceneUtil::WorkQueue> workQueue);

        void setTerrainPreloadPositions(const std::vector<PositionCellGrid>& positions);

        void syncTerrainLoad(Loading::Listener& listener);
        void abortTerrainPreloadExcept(const PositionCellGrid* exceptPos);
        bool isTerrainLoaded(const PositionCellGrid& position, double referenceTime) const;
        void setTerrain(Terrain::World* terrain);

        void reportStats(unsigned int frameNumber, osg::Stats& stats) const;

    private:
        void clearAllTasks();

        Resource::ResourceSystem* mResourceSystem;
        Resource::BulletShapeManager* mBulletShapeManager;
        Terrain::World* mTerrain;
        MWRender::LandManager* mLandManager;
        osg::ref_ptr<SceneUtil::WorkQueue> mWorkQueue;
        double mExpiryDelay;
        std::size_t mMinCacheSize = 0;
        std::size_t mMaxCacheSize = 0;
        bool mPreloadInstances;

        double mLastResourceCacheUpdate;

        struct PreloadEntry
        {
            PreloadEntry(double timestamp, osg::ref_ptr<SceneUtil::WorkItem> workItem)
                : mTimeStamp(timestamp)
                , mWorkItem(std::move(workItem))
            {
            }
            PreloadEntry()
                : mTimeStamp(0.0)
            {
            }

            double mTimeStamp;
            osg::ref_ptr<SceneUtil::WorkItem> mWorkItem;
        };
        typedef std::map<const MWWorld::CellStore*, PreloadEntry> PreloadMap;

        // Cells that are currently being preloaded, or have already finished preloading
        PreloadMap mPreloadCells;

        std::vector<osg::ref_ptr<Terrain::View>> mTerrainViews;
        std::vector<PositionCellGrid> mTerrainPreloadPositions;
        osg::ref_ptr<TerrainPreloadItem> mTerrainPreloadItem;
        osg::ref_ptr<SceneUtil::WorkItem> mUpdateCacheItem;

        std::vector<PositionCellGrid> mLoadedTerrainPositions;
        double mLoadedTerrainTimestamp;
        std::size_t mEvicted = 0;
        std::size_t mAdded = 0;
        std::size_t mExpired = 0;
        std::size_t mLoaded = 0;
    };

}

#endif
