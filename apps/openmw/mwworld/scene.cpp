#include "scene.hpp"

#include <OgreSceneNode.h>

#include <components/nif/niffile.hpp>

#include <libs/openengine/ogre/fader.hpp>

#include "../mwbase/environment.hpp"
#include "../mwbase/world.hpp" /// FIXME
#include "../mwbase/soundmanager.hpp"
#include "../mwbase/mechanicsmanager.hpp"
#include "../mwbase/windowmanager.hpp"

#include "physicssystem.hpp"
#include "player.hpp"
#include "localscripts.hpp"
#include "esmstore.hpp"
#include "class.hpp"
#include "cellfunctors.hpp"
#include "cellstore.hpp"

namespace
{
    void updateObjectLocalRotation (const MWWorld::Ptr& ptr, MWWorld::PhysicsSystem& physics,
                                    MWRender::RenderingManager& rendering)
    {
        if (ptr.getRefData().getBaseNode() != NULL)
        {
            Ogre::Quaternion worldRotQuat(Ogre::Radian(ptr.getRefData().getPosition().rot[2]), Ogre::Vector3::NEGATIVE_UNIT_Z);
            if (!ptr.getClass().isActor())
                worldRotQuat = Ogre::Quaternion(Ogre::Radian(ptr.getRefData().getPosition().rot[0]), Ogre::Vector3::NEGATIVE_UNIT_X)*
                        Ogre::Quaternion(Ogre::Radian(ptr.getRefData().getPosition().rot[1]), Ogre::Vector3::NEGATIVE_UNIT_Y)* worldRotQuat;

            float x = ptr.getRefData().getLocalRotation().rot[0];
            float y = ptr.getRefData().getLocalRotation().rot[1];
            float z = ptr.getRefData().getLocalRotation().rot[2];

            Ogre::Quaternion rot(Ogre::Radian(z), Ogre::Vector3::NEGATIVE_UNIT_Z);
            if (!ptr.getClass().isActor())
                rot = Ogre::Quaternion(Ogre::Radian(x), Ogre::Vector3::NEGATIVE_UNIT_X)*
                Ogre::Quaternion(Ogre::Radian(y), Ogre::Vector3::NEGATIVE_UNIT_Y)*rot;

            ptr.getRefData().getBaseNode()->setOrientation(worldRotQuat*rot);
            physics.rotateObject(ptr);
        }
    }

    struct InsertFunctor
    {
        MWWorld::CellStore& mCell;
        bool mRescale;
        Loading::Listener& mLoadingListener;
        MWWorld::PhysicsSystem& mPhysics;
        MWRender::RenderingManager& mRendering;

        InsertFunctor (MWWorld::CellStore& cell, bool rescale, Loading::Listener& loadingListener,
            MWWorld::PhysicsSystem& physics, MWRender::RenderingManager& rendering);

        bool operator() (const MWWorld::Ptr& ptr);
    };

    InsertFunctor::InsertFunctor (MWWorld::CellStore& cell, bool rescale,
        Loading::Listener& loadingListener, MWWorld::PhysicsSystem& physics,
        MWRender::RenderingManager& rendering)
    : mCell (cell), mRescale (rescale), mLoadingListener (loadingListener),
      mPhysics (physics), mRendering (rendering)
    {}

    bool InsertFunctor::operator() (const MWWorld::Ptr& ptr)
    {
        if (mRescale)
        {
            if (ptr.getCellRef().getScale()<0.5)
                ptr.getCellRef().setScale(0.5);
            else if (ptr.getCellRef().getScale()>2)
                ptr.getCellRef().setScale(2);
        }

        if (ptr.getRefData().getCount() && ptr.getRefData().isEnabled())
        {
            try
            {
                mRendering.addObject (ptr);
                ptr.getClass().insertObject (ptr, mPhysics);

                updateObjectLocalRotation(ptr, mPhysics, mRendering);
                MWBase::Environment::get().getWorld()->scaleObject (ptr, ptr.getCellRef().getScale());
                ptr.getClass().adjustPosition (ptr);
            }
            catch (const std::exception& e)
            {
                std::string error ("error during rendering: ");
                std::cerr << error + e.what() << std::endl;
            }
        }

        mLoadingListener.increaseProgress (1);

        return true;
    }
}


namespace MWWorld
{

    void Scene::updateObjectLocalRotation (const Ptr& ptr)
    {
        ::updateObjectLocalRotation(ptr, *mPhysics, mRendering);
    }

    void Scene::updateObjectRotation (const Ptr& ptr)
    {
        if(ptr.getRefData().getBaseNode() != 0)
        {
            mRendering.rotateObject(ptr);
            mPhysics->rotateObject(ptr);
        }
    }

    void Scene::update (float duration, bool paused)
    {
        if (mNeedMapUpdate)
        {
            // Note: exterior cell maps must be updated, even if they were visited before, because the set of surrounding cells might be different
            // (and objects in a different cell can "bleed" into another cells map if they cross the border)
            for (CellStoreCollection::iterator active = mActiveCells.begin(); active!=mActiveCells.end(); ++active)
                mRendering.requestMap(*active);
            mNeedMapUpdate = false;
        }

        mRendering.update (duration, paused);
    }

    void Scene::unloadCell (CellStoreCollection::iterator iter)
    {
        std::cout << "Unloading cell\n";
        ListAndResetHandles functor;

        (*iter)->forEach<ListAndResetHandles>(functor);
        {
            // silence annoying g++ warning
            for (std::vector<Ogre::SceneNode*>::const_iterator iter2 (functor.mHandles.begin());
                iter2!=functor.mHandles.end(); ++iter2)
            {
                Ogre::SceneNode* node = *iter2;
                mPhysics->removeObject (node->getName());
            }
        }

        if ((*iter)->getCell()->isExterior())
        {
            ESM::Land* land =
                MWBase::Environment::get().getWorld()->getStore().get<ESM::Land>().search(
                    (*iter)->getCell()->getGridX(),
                    (*iter)->getCell()->getGridY()
                );
            if (land)
                mPhysics->removeHeightField ((*iter)->getCell()->getGridX(), (*iter)->getCell()->getGridY());
        }

        mRendering.removeCell(*iter);

        MWBase::Environment::get().getWorld()->getLocalScripts().clearCell (*iter);

        MWBase::Environment::get().getMechanicsManager()->drop (*iter);

        MWBase::Environment::get().getSoundManager()->stopSound (*iter);
        mActiveCells.erase(*iter);
    }

    void Scene::loadCell (CellStore *cell, Loading::Listener* loadingListener)
    {
        std::pair<CellStoreCollection::iterator, bool> result = mActiveCells.insert(cell);

        if(result.second)
        {
            float verts = ESM::Land::LAND_SIZE;
            float worldsize = ESM::Land::REAL_SIZE;

            // Load terrain physics first...
            if (cell->getCell()->isExterior())
            {
                ESM::Land* land =
                    MWBase::Environment::get().getWorld()->getStore().get<ESM::Land>().search(
                        cell->getCell()->getGridX(),
                        cell->getCell()->getGridY()
                    );
                if (land) {
                    mPhysics->addHeightField (
                        land->mLandData->mHeights,
                        cell->getCell()->getGridX(),
                        cell->getCell()->getGridY(),
                        0,
                        worldsize / (verts-1),
                        verts)
                    ;
                }
            }

            cell->respawn();

            // ... then references. This is important for adjustPosition to work correctly.
            /// \todo rescale depending on the state of a new GMST
            insertCell (*cell, true, loadingListener);

            mRendering.cellAdded (cell);

            mRendering.configureAmbient(*cell);
        }

        // register local scripts
        // ??? Should this go into the above if block ???
        MWBase::Environment::get().getWorld()->getLocalScripts().addCell (cell);
    }

    void Scene::playerCellChange(CellStore *cell, const ESM::Position& pos, bool adjustPlayerPos)
    {
        MWBase::World *world = MWBase::Environment::get().getWorld();
        MWWorld::Ptr old = world->getPlayerPtr();
        world->getPlayer().setCell(cell);

        MWWorld::Ptr player = world->getPlayerPtr();
        mRendering.updatePlayerPtr(player);

        if (adjustPlayerPos) {
            world->moveObject(player, pos.pos[0], pos.pos[1], pos.pos[2]);

            float x = Ogre::Radian(pos.rot[0]).valueDegrees();
            float y = Ogre::Radian(pos.rot[1]).valueDegrees();
            float z = Ogre::Radian(pos.rot[2]).valueDegrees();
            world->rotateObject(player, x, y, z);

            player.getClass().adjustPosition(player);
        }

        MWBase::MechanicsManager *mechMgr =
            MWBase::Environment::get().getMechanicsManager();

        mechMgr->updateCell(old, player);
        mechMgr->watchActor(player);

        mRendering.updateTerrain();

        // Delay the map update until scripts have been given a chance to run.
        // If we don't do this, objects that should be disabled will still appear on the map.
        mNeedMapUpdate = true;

        MWBase::Environment::get().getWindowManager()->changeCell(mCurrentCell);
    }

    void Scene::changeToVoid()
    {
        CellStoreCollection::iterator active = mActiveCells.begin();
        while (active!=mActiveCells.end())
            unloadCell (active++);
        assert(mActiveCells.empty());
        mCurrentCell = NULL;
    }

    void Scene::changeCell (int X, int Y, const ESM::Position& position, bool adjustPlayerPos)
    {
        Nif::NIFFile::CacheLock cachelock;

        Loading::Listener* loadingListener = MWBase::Environment::get().getWindowManager()->getLoadingScreen();
        Loading::ScopedLoad load(loadingListener);

        mRendering.enableTerrain(true);

        std::string loadingExteriorText = "#{sLoadingMessage3}";
        loadingListener->setLabel(loadingExteriorText);

        CellStoreCollection::iterator active = mActiveCells.begin();

        // get the number of cells to unload
        int numUnload = 0;
        while (active!=mActiveCells.end())
        {
            if ((*active)->getCell()->isExterior())
            {
                if (std::abs (X-(*active)->getCell()->getGridX())<=1 &&
                    std::abs (Y-(*active)->getCell()->getGridY())<=1)
                {
                    // keep cells within the new 3x3 grid
                    ++active;
                    continue;
                }
            }
            ++active;
            ++numUnload;
        }

        active = mActiveCells.begin();
        while (active!=mActiveCells.end())
        {
            if ((*active)->getCell()->isExterior())
            {
                if (std::abs (X-(*active)->getCell()->getGridX())<=1 &&
                    std::abs (Y-(*active)->getCell()->getGridY())<=1)
                {
                    // keep cells within the new 3x3 grid
                    ++active;
                    continue;
                }
            }
            unloadCell (active++);
        }

        int refsToLoad = 0;
        // get the number of refs to load
        for (int x=X-1; x<=X+1; ++x)
            for (int y=Y-1; y<=Y+1; ++y)
            {
                CellStoreCollection::iterator iter = mActiveCells.begin();

                while (iter!=mActiveCells.end())
                {
                    assert ((*iter)->getCell()->isExterior());

                    if (x==(*iter)->getCell()->getGridX() &&
                        y==(*iter)->getCell()->getGridY())
                        break;

                    ++iter;
                }

                if (iter==mActiveCells.end())
                    refsToLoad += MWBase::Environment::get().getWorld()->getExterior(x, y)->count();
            }

        loadingListener->setProgressRange(refsToLoad);

        // Load cells
        for (int x=X-1; x<=X+1; ++x)
            for (int y=Y-1; y<=Y+1; ++y)
            {
                CellStoreCollection::iterator iter = mActiveCells.begin();

                while (iter!=mActiveCells.end())
                {
                    assert ((*iter)->getCell()->isExterior());

                    if (x==(*iter)->getCell()->getGridX() &&
                        y==(*iter)->getCell()->getGridY())
                        break;

                    ++iter;
                }

                if (iter==mActiveCells.end())
                {
                    CellStore *cell = MWBase::Environment::get().getWorld()->getExterior(x, y);

                    loadCell (cell, loadingListener);
                }
            }

        // find current cell
        CellStoreCollection::iterator iter = mActiveCells.begin();

        while (iter!=mActiveCells.end())
        {
            assert ((*iter)->getCell()->isExterior());

            if (X==(*iter)->getCell()->getGridX() &&
                Y==(*iter)->getCell()->getGridY())
                break;

            ++iter;
        }

        assert (iter!=mActiveCells.end());

        mCurrentCell = *iter;

        // adjust player
        playerCellChange (mCurrentCell, position, adjustPlayerPos);

        // Sky system
        MWBase::Environment::get().getWorld()->adjustSky();

        mCellChanged = true;
    }

    //We need the ogre renderer and a scene node.
    Scene::Scene (MWRender::RenderingManager& rendering, PhysicsSystem *physics)
    : mCurrentCell (0), mCellChanged (false), mPhysics(physics), mRendering(rendering), mNeedMapUpdate(false)
    {
    }

    Scene::~Scene()
    {
    }

    bool Scene::hasCellChanged() const
    {
        return mCellChanged;
    }

    const Scene::CellStoreCollection& Scene::getActiveCells() const
    {
        return mActiveCells;
    }

    void Scene::changeToInteriorCell (const std::string& cellName, const ESM::Position& position)
    {
        Nif::NIFFile::CacheLock lock;
        MWBase::Environment::get().getWorld ()->getFader ()->fadeOut(0.5);

        Loading::Listener* loadingListener = MWBase::Environment::get().getWindowManager()->getLoadingScreen();
        Loading::ScopedLoad load(loadingListener);

        mRendering.enableTerrain(false);

        std::string loadingInteriorText = "#{sLoadingMessage2}";
        loadingListener->setLabel(loadingInteriorText);

        CellStore *cell = MWBase::Environment::get().getWorld()->getInterior(cellName);
        bool loadcell = (mCurrentCell == NULL);
        if(!loadcell)
            loadcell = *mCurrentCell != *cell;

        if(!loadcell)
        {
            MWBase::World *world = MWBase::Environment::get().getWorld();
            world->moveObject(world->getPlayerPtr(), position.pos[0], position.pos[1], position.pos[2]);

            float x = Ogre::Radian(position.rot[0]).valueDegrees();
            float y = Ogre::Radian(position.rot[1]).valueDegrees();
            float z = Ogre::Radian(position.rot[2]).valueDegrees();
            world->rotateObject(world->getPlayerPtr(), x, y, z);

            world->getPlayerPtr().getClass().adjustPosition(world->getPlayerPtr());
            world->getFader()->fadeIn(0.5f);
            return;
        }

        std::cout << "Changing to interior\n";

        // remove active
        CellStoreCollection::iterator active = mActiveCells.begin();

        // count number of cells to unload
        int numUnload = 0;
        while (active!=mActiveCells.end())
        {
            ++active;
            ++numUnload;
        }

        // unload
        int current = 0;
        active = mActiveCells.begin();
        while (active!=mActiveCells.end())
        {
            unloadCell (active++);
            ++current;
        }

        int refsToLoad = cell->count();
        loadingListener->setProgressRange(refsToLoad);

        // Load cell.
        std::cout << "cellName: " << cell->getCell()->mName << std::endl;

        //Loading Interior loading text

        loadCell (cell, loadingListener);

        mCurrentCell = cell;

        // adjust fog
        mRendering.configureFog(*mCurrentCell);

        // adjust player
        playerCellChange (mCurrentCell, position);

        // Sky system
        MWBase::Environment::get().getWorld()->adjustSky();

        mCellChanged = true;
        MWBase::Environment::get().getWorld ()->getFader ()->fadeIn(0.5);
    }

    void Scene::changeToExteriorCell (const ESM::Position& position)
    {
        int x = 0;
        int y = 0;

        MWBase::Environment::get().getWorld()->positionToIndex (position.pos[0], position.pos[1], x, y);

        changeCell (x, y, position, true);
    }

    CellStore* Scene::getCurrentCell ()
    {
        return mCurrentCell;
    }

    void Scene::markCellAsUnchanged()
    {
        mCellChanged = false;
    }

    void Scene::insertCell (CellStore &cell, bool rescale, Loading::Listener* loadingListener)
    {
        InsertFunctor functor (cell, rescale, *loadingListener, *mPhysics, mRendering);
        cell.forEach (functor);
    }

    void Scene::addObjectToScene (const Ptr& ptr)
    {
        mRendering.addObject(ptr);
        ptr.getClass().insertObject(ptr, *mPhysics);
        MWBase::Environment::get().getWorld()->rotateObject(ptr, 0, 0, 0, true);
        MWBase::Environment::get().getWorld()->scaleObject(ptr, ptr.getCellRef().getScale());
    }

    void Scene::removeObjectFromScene (const Ptr& ptr)
    {
        MWBase::Environment::get().getMechanicsManager()->remove (ptr);
        MWBase::Environment::get().getSoundManager()->stopSound3D (ptr);
        mPhysics->removeObject (ptr.getRefData().getHandle());
        mRendering.removeObject (ptr);
    }

    bool Scene::isCellActive(const CellStore &cell)
    {
        CellStoreCollection::iterator active = mActiveCells.begin();
        while (active != mActiveCells.end()) {
            if (**active == cell) {
                return true;
            }
            ++active;
        }
        return false;
    }

    Ptr Scene::searchPtrViaHandle (const std::string& handle)
    {
        for (CellStoreCollection::const_iterator iter (mActiveCells.begin());
            iter!=mActiveCells.end(); ++iter)
            if (Ptr ptr = (*iter)->searchViaHandle (handle))
                return ptr;

        return Ptr();
    }

    Ptr Scene::searchPtrViaActorId (int actorId)
    {
        for (CellStoreCollection::const_iterator iter (mActiveCells.begin());
            iter!=mActiveCells.end(); ++iter)
            if (Ptr ptr = (*iter)->searchViaActorId (actorId))
                return ptr;

        return Ptr();
    }
}
