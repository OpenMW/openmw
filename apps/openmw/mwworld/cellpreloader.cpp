#include "cellpreloader.hpp"

#include <atomic>
#include <limits>

#include <components/debug/debuglog.hpp>
#include <components/resource/scenemanager.hpp>
#include <components/resource/resourcesystem.hpp>
#include <components/resource/bulletshapemanager.hpp>
#include <components/resource/keyframemanager.hpp>
#include <components/vfs/manager.hpp>
#include <components/misc/resourcehelpers.hpp>
#include <components/misc/stringops.hpp>
#include <components/terrain/world.hpp>
#include <components/sceneutil/unrefqueue.hpp>
#include <components/esm/loadcell.hpp>

#include "../mwrender/landmanager.hpp"

#include "cellstore.hpp"
#include "class.hpp"

namespace MWWorld
{

    struct ListModelsVisitor
    {
        ListModelsVisitor(std::vector<std::string>& out)
            : mOut(out)
        {
        }

        virtual bool operator()(const MWWorld::Ptr& ptr)
        {
            ptr.getClass().getModelsToPreload(ptr, mOut);

            return true;
        }

        virtual ~ListModelsVisitor() = default;

        std::vector<std::string>& mOut;
    };

    /// Worker thread item: preload models in a cell.
    class PreloadItem : public SceneUtil::WorkItem
    {
    public:
        /// Constructor to be called from the main thread.
        PreloadItem(MWWorld::CellStore* cell, Resource::SceneManager* sceneManager, Resource::BulletShapeManager* bulletShapeManager, Resource::KeyframeManager* keyframeManager, Terrain::World* terrain, MWRender::LandManager* landManager, bool preloadInstances)
            : mIsExterior(cell->getCell()->isExterior())
            , mX(cell->getCell()->getGridX())
            , mY(cell->getCell()->getGridY())
            , mSceneManager(sceneManager)
            , mBulletShapeManager(bulletShapeManager)
            , mKeyframeManager(keyframeManager)
            , mTerrain(terrain)
            , mLandManager(landManager)
            , mPreloadInstances(preloadInstances)
            , mAbort(false)
        {
            mTerrainView = mTerrain->createView();

            ListModelsVisitor visitor (mMeshes);
            cell->forEach(visitor);
        }

        void abort() override
        {
            mAbort = true;
        }

        /// Preload work to be called from the worker thread.
        void doWork() override
        {
            if (mIsExterior)
            {
                try
                {
                    mTerrain->cacheCell(mTerrainView.get(), mX, mY);
                    mPreloadedObjects.insert(mLandManager->getLand(mX, mY));
                }
                catch(std::exception&)
                {
                }
            }

            for (std::string& mesh: mMeshes)
            {
                if (mAbort)
                    break;

                try
                {
                    mesh = Misc::ResourceHelpers::correctActorModelPath(mesh, mSceneManager->getVFS());

                    bool animated = false;
                    size_t slashpos = mesh.find_last_of("/\\");
                    if (slashpos != std::string::npos && slashpos != mesh.size()-1)
                    {
                        Misc::StringUtils::lowerCaseInPlace(mesh);
                        if (mesh[slashpos+1] == 'x')
                        {
                            std::string kfname = mesh;
                            if(kfname.size() > 4 && kfname.compare(kfname.size()-4, 4, ".nif") == 0)
                            {
                                kfname.replace(kfname.size()-4, 4, ".kf");
                                if (mSceneManager->getVFS()->exists(kfname))
                                {
                                    mPreloadedObjects.insert(mKeyframeManager->get(kfname));
                                    animated = true;
                                }
                            }
                        }
                    }
                    if (mPreloadInstances && animated)
                        mPreloadedObjects.insert(mSceneManager->cacheInstance(mesh));
                    else
                        mPreloadedObjects.insert(mSceneManager->getTemplate(mesh));
                    if (mPreloadInstances)
                        mPreloadedObjects.insert(mBulletShapeManager->cacheInstance(mesh));
                    else
                        mPreloadedObjects.insert(mBulletShapeManager->getShape(mesh));

                }
                catch (std::exception&)
                {
                    // ignore error for now, would spam the log too much
                    // error will be shown when visiting the cell
                }
            }
        }

    private:
        typedef std::vector<std::string> MeshList;
        bool mIsExterior;
        int mX;
        int mY;
        MeshList mMeshes;
        Resource::SceneManager* mSceneManager;
        Resource::BulletShapeManager* mBulletShapeManager;
        Resource::KeyframeManager* mKeyframeManager;
        Terrain::World* mTerrain;
        MWRender::LandManager* mLandManager;
        bool mPreloadInstances;

        std::atomic<bool> mAbort;

        osg::ref_ptr<Terrain::View> mTerrainView;

        // keep a ref to the loaded objects to make sure it stays loaded as long as this cell is in the preloaded state
        std::set<osg::ref_ptr<const osg::Object> > mPreloadedObjects;
    };

    class TerrainPreloadItem : public SceneUtil::WorkItem
    {
    public:
        TerrainPreloadItem(const std::vector<osg::ref_ptr<Terrain::View> >& views, Terrain::World* world, const std::vector<CellPreloader::PositionCellGrid>& preloadPositions)
            : mAbort(false)
            , mProgress(views.size())
            , mProgressRange(0)
            , mTerrainViews(views)
            , mWorld(world)
            , mPreloadPositions(preloadPositions)
        {
        }

        bool storeViews(double referenceTime)
        {
            for (unsigned int i=0; i<mTerrainViews.size() && i<mPreloadPositions.size(); ++i)
                if (!mWorld->storeView(mTerrainViews[i], referenceTime))
                    return false;
            return true;
        }

        void doWork() override
        {
            for (unsigned int i=0; i<mTerrainViews.size() && i<mPreloadPositions.size() && !mAbort; ++i)
            {
                mTerrainViews[i]->reset();
                mWorld->preload(mTerrainViews[i], mPreloadPositions[i].first, mPreloadPositions[i].second, mAbort, mProgress[i], mProgressRange);
            }
        }

        void abort() override
        {
            mAbort = true;
        }

        int getProgress() const { return !mProgress.empty() ? mProgress[0].load() : 0; }
        int getProgressRange() const { return !mProgress.empty() && mProgress[0].load() ? mProgressRange : 0; }

    private:
        std::atomic<bool> mAbort;
        std::vector<std::atomic<int>> mProgress;
        int mProgressRange;
        std::vector<osg::ref_ptr<Terrain::View> > mTerrainViews;
        Terrain::World* mWorld;
        std::vector<CellPreloader::PositionCellGrid> mPreloadPositions;
    };

    /// Worker thread item: update the resource system's cache, effectively deleting unused entries.
    class UpdateCacheItem : public SceneUtil::WorkItem
    {
    public:
        UpdateCacheItem(Resource::ResourceSystem* resourceSystem, double referenceTime)
            : mReferenceTime(referenceTime)
            , mResourceSystem(resourceSystem)
        {
        }

        void doWork() override
        {
            mResourceSystem->updateCache(mReferenceTime);
        }

    private:
        double mReferenceTime;
        Resource::ResourceSystem* mResourceSystem;
    };

    CellPreloader::CellPreloader(Resource::ResourceSystem* resourceSystem, Resource::BulletShapeManager* bulletShapeManager, Terrain::World* terrain, MWRender::LandManager* landManager)
        : mResourceSystem(resourceSystem)
        , mBulletShapeManager(bulletShapeManager)
        , mTerrain(terrain)
        , mLandManager(landManager)
        , mExpiryDelay(0.0)
        , mMinCacheSize(0)
        , mMaxCacheSize(0)
        , mPreloadInstances(true)
        , mLastResourceCacheUpdate(0.0)
        , mStoreViewsFailCount(0)
    {
    }

    CellPreloader::~CellPreloader()
    {
        if (mTerrainPreloadItem)
        {
            mTerrainPreloadItem->abort();
            mTerrainPreloadItem->waitTillDone();
            mTerrainPreloadItem = nullptr;
        }

        if (mUpdateCacheItem)
        {
            mUpdateCacheItem->waitTillDone();
            mUpdateCacheItem = nullptr;
        }

        for (PreloadMap::iterator it = mPreloadCells.begin(); it != mPreloadCells.end();++it)
            it->second.mWorkItem->abort();

        for (PreloadMap::iterator it = mPreloadCells.begin(); it != mPreloadCells.end();++it)
            it->second.mWorkItem->waitTillDone();

        mPreloadCells.clear();
    }

    void CellPreloader::preload(CellStore *cell, double timestamp)
    {
        if (!mWorkQueue)
        {
            Log(Debug::Error) << "Error: can't preload, no work queue set";
            return;
        }
        if (cell->getState() == CellStore::State_Unloaded)
        {
            Log(Debug::Error) << "Error: can't preload objects for unloaded cell";
            return;
        }

        PreloadMap::iterator found = mPreloadCells.find(cell);
        if (found != mPreloadCells.end())
        {
            // already preloaded, nothing to do other than updating the timestamp
            found->second.mTimeStamp = timestamp;
            return;
        }

        while (mPreloadCells.size() >= mMaxCacheSize)
        {
            // throw out oldest cell to make room
            PreloadMap::iterator oldestCell = mPreloadCells.begin();
            double oldestTimestamp = std::numeric_limits<double>::max();
            double threshold = 1.0; // seconds
            for (PreloadMap::iterator it = mPreloadCells.begin(); it != mPreloadCells.end(); ++it)
            {
                if (it->second.mTimeStamp < oldestTimestamp)
                {
                    oldestTimestamp = it->second.mTimeStamp;
                    oldestCell = it;
                }
            }

            if (oldestTimestamp + threshold < timestamp)
            {
                oldestCell->second.mWorkItem->abort();
                mPreloadCells.erase(oldestCell);
            }
            else
                return;
        }

        osg::ref_ptr<PreloadItem> item (new PreloadItem(cell, mResourceSystem->getSceneManager(), mBulletShapeManager, mResourceSystem->getKeyframeManager(), mTerrain, mLandManager, mPreloadInstances));
        mWorkQueue->addWorkItem(item);

        mPreloadCells[cell] = PreloadEntry(timestamp, item);
    }

    void CellPreloader::notifyLoaded(CellStore *cell)
    {
        PreloadMap::iterator found = mPreloadCells.find(cell);
        if (found != mPreloadCells.end())
        {
            // do the deletion in the background thread
            if (found->second.mWorkItem)
            {
                found->second.mWorkItem->abort();
                mUnrefQueue->push(mPreloadCells[cell].mWorkItem);
            }

            mPreloadCells.erase(found);
        }
    }

    void CellPreloader::clear()
    {
        for (PreloadMap::iterator it = mPreloadCells.begin(); it != mPreloadCells.end();)
        {
            if (it->second.mWorkItem)
            {
                it->second.mWorkItem->abort();
                mUnrefQueue->push(it->second.mWorkItem);
            }

            mPreloadCells.erase(it++);
        }
    }

    void CellPreloader::updateCache(double timestamp)
    {
        for (PreloadMap::iterator it = mPreloadCells.begin(); it != mPreloadCells.end();)
        {
            if (mPreloadCells.size() >= mMinCacheSize && it->second.mTimeStamp < timestamp - mExpiryDelay)
            {
                if (it->second.mWorkItem)
                {
                    it->second.mWorkItem->abort();
                    mUnrefQueue->push(it->second.mWorkItem);
                }
                mPreloadCells.erase(it++);
            }
            else
                ++it;
        }

        if (timestamp - mLastResourceCacheUpdate > 1.0 && (!mUpdateCacheItem || mUpdateCacheItem->isDone()))
        {
            // the resource cache is cleared from the worker thread so that we're not holding up the main thread with delete operations
            mUpdateCacheItem = new UpdateCacheItem(mResourceSystem, timestamp);
            mWorkQueue->addWorkItem(mUpdateCacheItem, true);
            mLastResourceCacheUpdate = timestamp;
        }

        if (mTerrainPreloadItem && mTerrainPreloadItem->isDone())
        {
            if (!mTerrainPreloadItem->storeViews(timestamp))
            {
                if (++mStoreViewsFailCount > 100)
                {
                    OSG_ALWAYS << "paging views are rebuilt every frame, please check for faulty enable/disable scripts." << std::endl;
                    mStoreViewsFailCount = 0;
                }
                setTerrainPreloadPositions(std::vector<PositionCellGrid>());
            }
            else
                mStoreViewsFailCount = 0;
            mTerrainPreloadItem = nullptr;
        }
    }

    void CellPreloader::setExpiryDelay(double expiryDelay)
    {
        mExpiryDelay = expiryDelay;
    }

    void CellPreloader::setMinCacheSize(unsigned int num)
    {
        mMinCacheSize = num;
    }

    void CellPreloader::setMaxCacheSize(unsigned int num)
    {
        mMaxCacheSize = num;
    }

    void CellPreloader::setPreloadInstances(bool preload)
    {
        mPreloadInstances = preload;
    }

    unsigned int CellPreloader::getMaxCacheSize() const
    {
        return mMaxCacheSize;
    }

    void CellPreloader::setWorkQueue(osg::ref_ptr<SceneUtil::WorkQueue> workQueue)
    {
        mWorkQueue = workQueue;
    }

    void CellPreloader::setUnrefQueue(SceneUtil::UnrefQueue* unrefQueue)
    {
        mUnrefQueue = unrefQueue;
    }

    bool CellPreloader::syncTerrainLoad(const std::vector<CellPreloader::PositionCellGrid> &positions, int& progress, int& progressRange, double timestamp)
    {
        if (!mTerrainPreloadItem)
            return true;
        else if (mTerrainPreloadItem->isDone())
        {
            if (mTerrainPreloadItem->storeViews(timestamp))
            {
                mTerrainPreloadItem = nullptr;
                return true;
            }
            else
            {
                setTerrainPreloadPositions(std::vector<CellPreloader::PositionCellGrid>());
                setTerrainPreloadPositions(positions);
                return false;
            }
        }
        else
        {
            progress = mTerrainPreloadItem->getProgress();
            progressRange = mTerrainPreloadItem->getProgressRange();
            return false;
        }
    }

    void CellPreloader::abortTerrainPreloadExcept(const CellPreloader::PositionCellGrid *exceptPos)
    {
        const float resetThreshold = ESM::Land::REAL_SIZE;
        for (const auto& pos : mTerrainPreloadPositions)
            if (exceptPos && (pos.first-exceptPos->first).length2() < resetThreshold*resetThreshold && pos.second == exceptPos->second)
                return;
        if (mTerrainPreloadItem && !mTerrainPreloadItem->isDone())
        {
            mTerrainPreloadItem->abort();
            mTerrainPreloadItem->waitTillDone();
        }
        setTerrainPreloadPositions(std::vector<CellPreloader::PositionCellGrid>());
    }

    bool contains(const std::vector<CellPreloader::PositionCellGrid>& container, const std::vector<CellPreloader::PositionCellGrid>& contained)
    {
        for (const auto& pos : contained)
        {
            bool found = false;
            for (const auto& pos2 : container)
            {
                if ((pos.first-pos2.first).length2() < 1 && pos.second == pos2.second)
                {
                    found = true;
                    break;
                }
            }
            if (!found) return false;
        }
        return true;
    }

    void CellPreloader::setTerrainPreloadPositions(const std::vector<CellPreloader::PositionCellGrid> &positions)
    {
        if (positions.empty())
            mTerrainPreloadPositions.clear();
        else if (contains(mTerrainPreloadPositions, positions))
            return;
        if (mTerrainPreloadItem && !mTerrainPreloadItem->isDone())
            return;
        else
        {
            if (mTerrainViews.size() > positions.size())
            {
                for (unsigned int i=positions.size(); i<mTerrainViews.size(); ++i)
                    mUnrefQueue->push(mTerrainViews[i]);
                mTerrainViews.resize(positions.size());
            }
            else if (mTerrainViews.size() < positions.size())
            {
                for (unsigned int i=mTerrainViews.size(); i<positions.size(); ++i)
                    mTerrainViews.emplace_back(mTerrain->createView());
            }

            mTerrainPreloadPositions = positions;
            if (!positions.empty())
            {
                mTerrainPreloadItem = new TerrainPreloadItem(mTerrainViews, mTerrain, positions);
                mWorkQueue->addWorkItem(mTerrainPreloadItem);
            }
        }
    }

}
