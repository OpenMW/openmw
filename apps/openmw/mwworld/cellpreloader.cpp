#include "cellpreloader.hpp"

#include <iostream>

#include <components/resource/scenemanager.hpp>
#include <components/resource/resourcesystem.hpp>
#include <components/resource/bulletshapemanager.hpp>
#include <components/resource/keyframemanager.hpp>
#include <components/misc/resourcehelpers.hpp>
#include <components/nifosg/nifloader.hpp>
#include <components/terrain/world.hpp>

#include "../mwbase/environment.hpp"
#include "../mwbase/world.hpp"

#include "../mwworld/inventorystore.hpp"
#include "../mwworld/esmstore.hpp"

#include "cellstore.hpp"
#include "manualref.hpp"
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

        std::vector<std::string>& mOut;
    };

    /// Worker thread item: preload models in a cell.
    class PreloadItem : public SceneUtil::WorkItem
    {
    public:
        /// Constructor to be called from the main thread.
        PreloadItem(MWWorld::CellStore* cell, Resource::SceneManager* sceneManager, Resource::BulletShapeManager* bulletShapeManager, Resource::KeyframeManager* keyframeManager, Terrain::World* terrain)
            : mIsExterior(cell->getCell()->isExterior())
            , mX(cell->getCell()->getGridX())
            , mY(cell->getCell()->getGridY())
            , mSceneManager(sceneManager)
            , mBulletShapeManager(bulletShapeManager)
            , mKeyframeManager(keyframeManager)
            , mTerrain(terrain)
        {
            osg::Timer timer;
            ListModelsVisitor visitor (mMeshes);
            if (cell->getState() == MWWorld::CellStore::State_Loaded)
            {
                cell->forEach(visitor);
            }
            else
            {
                const std::vector<std::string>& objectIds = cell->getPreloadedIds();

                // could possibly build the model list in the worker thread if we manage to make the Store thread safe
                for (std::vector<std::string>::const_iterator it = objectIds.begin(); it != objectIds.end(); ++it)
                {
                    MWWorld::ManualRef ref(MWBase::Environment::get().getWorld()->getStore(), *it);
                    std::string model = ref.getPtr().getClass().getModel(ref.getPtr());
                    if (!model.empty())
                        mMeshes.push_back(model);
                }
            }
            std::cout << "listed models in " << timer.time_m() << std::endl;
        }

        /// Preload work to be called from the worker thread.
        virtual void doWork()
        {
            // TODO: make CellStore::loadRefs thread safe so we can call it from here

            osg::Timer preloadTimer;
            for (MeshList::const_iterator it = mMeshes.begin(); it != mMeshes.end(); ++it)
            {
                try
                {
                    std::string mesh  = *it;
                    mesh = Misc::ResourceHelpers::correctActorModelPath(mesh, mSceneManager->getVFS());

                    //std::cout << "preloading " << mesh << std::endl;

                    mPreloadedObjects.push_back(mSceneManager->cacheInstance(mesh));
                    mPreloadedObjects.push_back(mBulletShapeManager->cacheInstance(mesh));

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
                                mPreloadedObjects.push_back(mKeyframeManager->get(kfname));
                            }

                        }
                    }
                }
                catch (std::exception& e)
                {
                    // ignore error for now, would spam the log too much
                    // error will be shown when visiting the cell
                }
            }

            if (mIsExterior)
            {
                try
                {
                    mPreloadedObjects.push_back(mTerrain->cacheCell(mX, mY));
                }
                catch(std::exception& e)
                {
                }
            }

            std::cout << "preloaded " << mPreloadedObjects.size() << " objects in " << preloadTimer.time_m() << std::endl;
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

        // keep a ref to the loaded objects to make sure it stays loaded as long as this cell is in the preloaded state
        std::vector<osg::ref_ptr<const osg::Object> > mPreloadedObjects;
    };

    /// Worker thread item: update the resource system's cache, effectively deleting unused entries.
    class UpdateCacheItem : public SceneUtil::WorkItem
    {
    public:
        UpdateCacheItem(Resource::ResourceSystem* resourceSystem, Terrain::World* terrain, double referenceTime)
            : mReferenceTime(referenceTime)
            , mResourceSystem(resourceSystem)
            , mTerrain(terrain)
        {
        }

        virtual void doWork()
        {
            osg::Timer timer;
            mResourceSystem->updateCache(mReferenceTime);

            mTerrain->updateCache();
            std::cout << "cleared cache in " << timer.time_m() << std::endl;
        }

    private:
        double mReferenceTime;
        Resource::ResourceSystem* mResourceSystem;
        Terrain::World* mTerrain;
    };

    CellPreloader::CellPreloader(Resource::ResourceSystem* resourceSystem, Resource::BulletShapeManager* bulletShapeManager, Terrain::World* terrain)
        : mResourceSystem(resourceSystem)
        , mBulletShapeManager(bulletShapeManager)
        , mTerrain(terrain)
        , mExpiryDelay(0.0)
    {
    }

    void CellPreloader::preload(CellStore *cell, double timestamp)
    {
        if (!mWorkQueue)
        {
            std::cerr << "can't preload, no work queue set " << std::endl;
            return;
        }
        if (cell->getState() == CellStore::State_Unloaded)
        {
            std::cerr << "can't preload objects for unloaded cell" << std::endl;
            return;
        }

        PreloadMap::iterator found = mPreloadCells.find(cell);
        if (found != mPreloadCells.end())
        {
            // already preloaded, nothing to do other than updating the timestamp
            found->second.mTimeStamp = timestamp;
            return;
        }

        osg::ref_ptr<PreloadItem> item (new PreloadItem(cell, mResourceSystem->getSceneManager(), mBulletShapeManager, mResourceSystem->getKeyframeManager(), mTerrain));
        mWorkQueue->addWorkItem(item);

        mPreloadCells[cell] = PreloadEntry(timestamp, item);
    }

    void CellPreloader::notifyLoaded(CellStore *cell)
    {
        mPreloadCells.erase(cell);
    }

    void CellPreloader::updateCache(double timestamp)
    {
        // TODO: add settings for a minimum/maximum size of the cache

        for (PreloadMap::iterator it = mPreloadCells.begin(); it != mPreloadCells.end();)
        {
            if (it->second.mTimeStamp < timestamp - mExpiryDelay)
                mPreloadCells.erase(it++);
            else
                ++it;
        }

        // the resource cache is cleared from the worker thread so that we're not holding up the main thread with delete operations
        mWorkQueue->addWorkItem(new UpdateCacheItem(mResourceSystem, mTerrain, timestamp));
    }

    void CellPreloader::setExpiryDelay(double expiryDelay)
    {
        mExpiryDelay = expiryDelay;
    }

    void CellPreloader::setWorkQueue(osg::ref_ptr<SceneUtil::WorkQueue> workQueue)
    {
        mWorkQueue = workQueue;
    }

}
