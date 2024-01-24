#include "cellpreloader.hpp"

#include <array>
#include <atomic>
#include <limits>

#include <components/debug/debuglog.hpp>
#include <components/esm3/loadcell.hpp>
#include <components/loadinglistener/reporter.hpp>
#include <components/misc/constants.hpp>
#include <components/misc/resourcehelpers.hpp>
#include <components/misc/strings/lower.hpp>
#include <components/resource/bulletshapemanager.hpp>
#include <components/resource/keyframemanager.hpp>
#include <components/resource/resourcesystem.hpp>
#include <components/resource/scenemanager.hpp>
#include <components/terrain/view.hpp>
#include <components/terrain/world.hpp>
#include <components/vfs/manager.hpp>

#include "../mwrender/landmanager.hpp"

#include "cellstore.hpp"
#include "class.hpp"

namespace
{
    template <class Contained>
    bool contains(const std::vector<MWWorld::CellPreloader::PositionCellGrid>& container, const Contained& contained,
        float tolerance)
    {
        for (const auto& pos : contained)
        {
            bool found = false;
            for (const auto& pos2 : container)
            {
                if ((pos.first - pos2.first).length2() < tolerance * tolerance && pos.second == pos2.second)
                {
                    found = true;
                    break;
                }
            }
            if (!found)
                return false;
        }
        return true;
    }
}

namespace MWWorld
{

    struct ListModelsVisitor
    {
        bool operator()(const MWWorld::ConstPtr& ptr)
        {
            ptr.getClass().getModelsToPreload(ptr, mOut);

            return true;
        }

        std::vector<std::string>& mOut;
    };

    /// Worker thread item: preload models in a cell.
    class PreloadItem : public SceneUtil::WorkItem
    {
    public:
        /// Constructor to be called from the main thread.
        PreloadItem(MWWorld::CellStore* cell, Resource::SceneManager* sceneManager,
            Resource::BulletShapeManager* bulletShapeManager, Resource::KeyframeManager* keyframeManager,
            Terrain::World* terrain, MWRender::LandManager* landManager, bool preloadInstances)
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

            ListModelsVisitor visitor{ mMeshes };
            cell->forEachConst(visitor);
        }

        void abort() override { mAbort = true; }

        /// Preload work to be called from the worker thread.
        void doWork() override
        {
            if (mIsExterior)
            {
                try
                {
                    mTerrain->cacheCell(mTerrainView.get(), mX, mY);
                    mPreloadedObjects.insert(
                        mLandManager->getLand(ESM::ExteriorCellLocation(mX, mY, ESM::Cell::sDefaultWorldspaceId)));
                }
                catch (std::exception&)
                {
                }
            }

            for (std::string& mesh : mMeshes)
            {
                if (mAbort)
                    break;

                try
                {
                    mesh = Misc::ResourceHelpers::correctActorModelPath(mesh, mSceneManager->getVFS());

                    size_t slashpos = mesh.find_last_of("/\\");
                    if (slashpos != std::string::npos && slashpos != mesh.size() - 1)
                    {
                        Misc::StringUtils::lowerCaseInPlace(mesh);
                        if (mesh[slashpos + 1] == 'x')
                        {
                            if (mesh.size() > 4 && mesh.ends_with(".nif"))
                            {
                                std::string kfname = mesh;
                                kfname.replace(kfname.size() - 4, 4, ".kf");
                                if (mSceneManager->getVFS()->exists(kfname))
                                    mPreloadedObjects.insert(mKeyframeManager->get(kfname));
                            }
                        }
                    }
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
        std::set<osg::ref_ptr<const osg::Object>> mPreloadedObjects;
    };

    class TerrainPreloadItem : public SceneUtil::WorkItem
    {
    public:
        TerrainPreloadItem(const std::vector<osg::ref_ptr<Terrain::View>>& views, Terrain::World* world,
            const std::vector<CellPreloader::PositionCellGrid>& preloadPositions)
            : mAbort(false)
            , mTerrainViews(views)
            , mWorld(world)
            , mPreloadPositions(preloadPositions)
        {
        }

        void doWork() override
        {
            for (unsigned int i = 0; i < mTerrainViews.size() && i < mPreloadPositions.size() && !mAbort; ++i)
            {
                mTerrainViews[i]->reset();
                mWorld->preload(mTerrainViews[i], mPreloadPositions[i].first, mPreloadPositions[i].second, mAbort,
                    mLoadingReporter);
            }
            mLoadingReporter.complete();
        }

        void abort() override { mAbort = true; }

        void wait(Loading::Listener& listener) const { mLoadingReporter.wait(listener); }

    private:
        std::atomic<bool> mAbort;
        std::vector<osg::ref_ptr<Terrain::View>> mTerrainViews;
        Terrain::World* mWorld;
        std::vector<CellPreloader::PositionCellGrid> mPreloadPositions;
        Loading::Reporter mLoadingReporter;
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

        void doWork() override { mResourceSystem->updateCache(mReferenceTime); }

    private:
        double mReferenceTime;
        Resource::ResourceSystem* mResourceSystem;
    };

    CellPreloader::CellPreloader(Resource::ResourceSystem* resourceSystem,
        Resource::BulletShapeManager* bulletShapeManager, Terrain::World* terrain, MWRender::LandManager* landManager)
        : mResourceSystem(resourceSystem)
        , mBulletShapeManager(bulletShapeManager)
        , mTerrain(terrain)
        , mLandManager(landManager)
        , mExpiryDelay(0.0)
        , mMinCacheSize(0)
        , mMaxCacheSize(0)
        , mPreloadInstances(true)
        , mLastResourceCacheUpdate(0.0)
        , mLoadedTerrainTimestamp(0.0)
    {
    }

    CellPreloader::~CellPreloader()
    {
        clearAllTasks();
    }

    void CellPreloader::preload(CellStore& cell, double timestamp)
    {
        if (!mWorkQueue)
        {
            Log(Debug::Error) << "Error: can't preload, no work queue set";
            return;
        }
        if (cell.getState() == CellStore::State_Unloaded)
        {
            Log(Debug::Error) << "Error: can't preload objects for unloaded cell";
            return;
        }

        PreloadMap::iterator found = mPreloadCells.find(&cell);
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

        osg::ref_ptr<PreloadItem> item(new PreloadItem(&cell, mResourceSystem->getSceneManager(), mBulletShapeManager,
            mResourceSystem->getKeyframeManager(), mTerrain, mLandManager, mPreloadInstances));
        mWorkQueue->addWorkItem(item);

        mPreloadCells[&cell] = PreloadEntry(timestamp, item);
    }

    void CellPreloader::notifyLoaded(CellStore* cell)
    {
        PreloadMap::iterator found = mPreloadCells.find(cell);
        if (found != mPreloadCells.end())
        {
            if (found->second.mWorkItem)
            {
                found->second.mWorkItem->abort();
                found->second.mWorkItem = nullptr;
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
                it->second.mWorkItem = nullptr;
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
                    it->second.mWorkItem = nullptr;
                }
                mPreloadCells.erase(it++);
            }
            else
                ++it;
        }

        if (timestamp - mLastResourceCacheUpdate > 1.0 && (!mUpdateCacheItem || mUpdateCacheItem->isDone()))
        {
            // the resource cache is cleared from the worker thread so that we're not holding up the main thread with
            // delete operations
            mUpdateCacheItem = new UpdateCacheItem(mResourceSystem, timestamp);
            mWorkQueue->addWorkItem(mUpdateCacheItem, true);
            mLastResourceCacheUpdate = timestamp;
        }

        if (mTerrainPreloadItem && mTerrainPreloadItem->isDone())
        {
            mLoadedTerrainPositions = mTerrainPreloadPositions;
            mLoadedTerrainTimestamp = timestamp;
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

    void CellPreloader::syncTerrainLoad(Loading::Listener& listener)
    {
        if (mTerrainPreloadItem != nullptr && !mTerrainPreloadItem->isDone())
            mTerrainPreloadItem->wait(listener);
    }

    void CellPreloader::abortTerrainPreloadExcept(const CellPreloader::PositionCellGrid* exceptPos)
    {
        if (exceptPos && contains(mTerrainPreloadPositions, std::array{ *exceptPos }, Constants::CellSizeInUnits))
            return;
        if (mTerrainPreloadItem && !mTerrainPreloadItem->isDone())
        {
            mTerrainPreloadItem->abort();
            mTerrainPreloadItem->waitTillDone();
        }
        setTerrainPreloadPositions(std::vector<CellPreloader::PositionCellGrid>());
    }

    void CellPreloader::setTerrainPreloadPositions(const std::vector<CellPreloader::PositionCellGrid>& positions)
    {
        if (positions.empty())
        {
            mTerrainPreloadPositions.clear();
            mLoadedTerrainPositions.clear();
        }
        else if (contains(mTerrainPreloadPositions, positions, 128.f))
            return;
        if (mTerrainPreloadItem && !mTerrainPreloadItem->isDone())
            return;
        else
        {
            if (mTerrainViews.size() > positions.size())
                mTerrainViews.resize(positions.size());
            else if (mTerrainViews.size() < positions.size())
            {
                for (unsigned int i = mTerrainViews.size(); i < positions.size(); ++i)
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

    bool CellPreloader::isTerrainLoaded(const CellPreloader::PositionCellGrid& position, double referenceTime) const
    {
        return mLoadedTerrainTimestamp + mResourceSystem->getSceneManager()->getExpiryDelay() > referenceTime
            && contains(mLoadedTerrainPositions, std::array{ position }, Constants::CellSizeInUnits);
    }

    void CellPreloader::setTerrain(Terrain::World* terrain)
    {
        if (terrain != mTerrain)
        {
            clearAllTasks();
            mTerrain = terrain;
        }
    }

    void CellPreloader::clearAllTasks()
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

        for (PreloadMap::iterator it = mPreloadCells.begin(); it != mPreloadCells.end(); ++it)
            it->second.mWorkItem->abort();

        for (PreloadMap::iterator it = mPreloadCells.begin(); it != mPreloadCells.end(); ++it)
            it->second.mWorkItem->waitTillDone();

        mPreloadCells.clear();
    }

}
