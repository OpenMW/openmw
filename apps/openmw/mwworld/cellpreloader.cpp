#include "cellpreloader.hpp"

#include <iostream>

#include <components/resource/scenemanager.hpp>
#include <components/resource/resourcesystem.hpp>
#include <components/resource/bulletshapemanager.hpp>
#include <components/misc/resourcehelpers.hpp>

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
            std::string model = ptr.getClass().getModel(ptr);
            if (!model.empty())
                mOut.push_back(model);

            // TODO: preload NPC body parts (mHead / mHair)

            // FIXME: use const version of InventoryStore functions once they are available
            if (ptr.getClass().hasInventoryStore(ptr))
            {
                MWWorld::InventoryStore& invStore = ptr.getClass().getInventoryStore(ptr);
                for (int slot = 0; slot < MWWorld::InventoryStore::Slots; ++slot)
                {
                    MWWorld::ContainerStoreIterator equipped = invStore.getSlot(slot);
                    if (equipped != invStore.end())
                    {
                        std::vector<ESM::PartReference> parts;
                        if(equipped->getTypeName() == typeid(ESM::Clothing).name())
                        {
                            const ESM::Clothing *clothes = equipped->get<ESM::Clothing>()->mBase;
                            parts = clothes->mParts.mParts;
                        }
                        else if(equipped->getTypeName() == typeid(ESM::Armor).name())
                        {
                            const ESM::Armor *armor = equipped->get<ESM::Armor>()->mBase;
                            parts = armor->mParts.mParts;
                        }
                        else
                        {
                            model = equipped->getClass().getModel(*equipped);
                            if (!model.empty())
                                mOut.push_back(model);
                        }

                        for (std::vector<ESM::PartReference>::const_iterator it = parts.begin(); it != parts.end(); ++it)
                        {
                            const ESM::BodyPart* part = MWBase::Environment::get().getWorld()->getStore().get<ESM::BodyPart>().search(it->mMale);
                            if (part && !part->mModel.empty())
                                mOut.push_back("meshes/"+part->mModel);
                            part = MWBase::Environment::get().getWorld()->getStore().get<ESM::BodyPart>().search(it->mFemale);
                            if (part && !part->mModel.empty())
                                mOut.push_back("meshes/"+part->mModel);
                        }
                    }
                }
            }

            return true;
        }

        std::vector<std::string>& mOut;
    };

    /// Worker thread item: preload models in a cell.
    class PreloadItem : public SceneUtil::WorkItem
    {
    public:
        /// Constructor to be called from the main thread.
        PreloadItem(MWWorld::CellStore* cell, Resource::SceneManager* sceneManager, Resource::BulletShapeManager* bulletShapeManager)
            : mSceneManager(sceneManager)
            , mBulletShapeManager(bulletShapeManager)
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

                    mPreloadedNodes.push_back(mSceneManager->getTemplate(*it));
                    mPreloadedShapes.push_back(mBulletShapeManager->getShape(*it));

                    // TODO: load .kf

                    // TODO: do a createInstance() and hold on to it since we can make use of it when the cell goes active
                }
                catch (std::exception& e)
                {
                    // ignore error for now, would spam the log too much
                    // error will be shown when visiting the cell
                }
            }
            std::cout << "preloaded " << mPreloadedNodes.size() << " nodes in " << preloadTimer.time_m() << std::endl;
        }

    private:
        typedef std::vector<std::string> MeshList;
        MeshList mMeshes;
        Resource::SceneManager* mSceneManager;
        Resource::BulletShapeManager* mBulletShapeManager;

        // keep a ref to the loaded object to make sure it stays loaded as long as this cell is in the preloaded state
        std::vector<osg::ref_ptr<const osg::Node> > mPreloadedNodes;
        std::vector<osg::ref_ptr<const Resource::BulletShape> > mPreloadedShapes;
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

        virtual void doWork()
        {
            osg::Timer timer;
            mResourceSystem->updateCache(mReferenceTime);
            std::cout << "cleared cache in " << timer.time_m() << std::endl;
        }

    private:
        double mReferenceTime;
        Resource::ResourceSystem* mResourceSystem;
    };

    CellPreloader::CellPreloader(Resource::ResourceSystem* resourceSystem, Resource::BulletShapeManager* bulletShapeManager)
        : mResourceSystem(resourceSystem)
        , mBulletShapeManager(bulletShapeManager)
        , mExpiryDelay(0.0)
    {
        mWorkQueue = new SceneUtil::WorkQueue;
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

        osg::ref_ptr<PreloadItem> item (new PreloadItem(cell, mResourceSystem->getSceneManager(), mBulletShapeManager));
        mWorkQueue->addWorkItem(item);

        mPreloadCells[cell] = PreloadEntry(timestamp, item);
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
        mWorkQueue->addWorkItem(new UpdateCacheItem(mResourceSystem, timestamp));
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
