#include "scene.hpp"

#include <components/nif/niffile.hpp>

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

namespace
{

    template<typename T>
    void insertCellRefList(MWRender::RenderingManager& rendering,
        T& cellRefList, MWWorld::CellStore &cell, MWWorld::PhysicsSystem& physics, bool rescale)
    {
        if (!cellRefList.mList.empty())
        {
            const MWWorld::Class& class_ =
                MWWorld::Class::get (MWWorld::Ptr (&*cellRefList.mList.begin(), &cell));
            int current = 0;
            for (typename T::List::iterator it = cellRefList.mList.begin();
                it != cellRefList.mList.end(); it++)
            {
                if (rescale)
                {
                    if (it->mRef.mScale<0.5)
                        it->mRef.mScale = 0.5;
                    else if (it->mRef.mScale>2)
                        it->mRef.mScale = 2;
                }

                ++current;

                if (it->mData.getCount() && it->mData.isEnabled())
                {
                    MWWorld::Ptr ptr (&*it, &cell);

                    try
                    {
                        rendering.addObject(ptr);
                        class_.insertObject(ptr, physics);
                        MWBase::Environment::get().getWorld()->rotateObject(ptr, 0, 0, 0, true);
                        MWBase::Environment::get().getWorld()->scaleObject(ptr, ptr.getCellRef().mScale);
                    }
                    catch (const std::exception& e)
                    {
                        std::string error ("error during rendering: ");
                        std::cerr << error + e.what() << std::endl;
                    }
                }
            }
        }
    }

}


namespace MWWorld
{

    void Scene::update (float duration, bool paused){
        mRendering.update (duration, paused);
    }

    void Scene::unloadCell (CellStoreCollection::iterator iter)
    {
        std::cout << "Unloading cell\n";
        ListHandles functor;

        (*iter)->forEach<ListHandles>(functor);
        {
            // silence annoying g++ warning
            for (std::vector<Ogre::SceneNode*>::const_iterator iter2 (functor.mHandles.begin());
                iter2!=functor.mHandles.end(); ++iter2){
                 Ogre::SceneNode* node = *iter2;
                mPhysics->removeObject (node->getName());
            }

            if ((*iter)->mCell->isExterior())
            {
                ESM::Land* land =
                    MWBase::Environment::get().getWorld()->getStore().get<ESM::Land>().search(
                        (*iter)->mCell->getGridX(),
                        (*iter)->mCell->getGridY()
                    );
                if (land)
                    mPhysics->removeHeightField( (*iter)->mCell->getGridX(), (*iter)->mCell->getGridY() );
            }
        }

        mRendering.removeCell(*iter);
       //mPhysics->removeObject("Unnamed_43");

        MWBase::Environment::get().getWorld()->getLocalScripts().clearCell (*iter);
        MWBase::Environment::get().getMechanicsManager()->drop (*iter);
        MWBase::Environment::get().getSoundManager()->stopSound (*iter);
        mActiveCells.erase(*iter);
    }

    void Scene::loadCell (Ptr::CellStore *cell)
    {
        // register local scripts
        MWBase::Environment::get().getWorld()->getLocalScripts().addCell (cell);
        std::pair<CellStoreCollection::iterator, bool> result = mActiveCells.insert(cell);

        if(result.second)
        {
            /// \todo rescale depending on the state of a new GMST
            insertCell (*cell, true);

            mRendering.cellAdded (cell);

            float verts = ESM::Land::LAND_SIZE;
            float worldsize = ESM::Land::REAL_SIZE;

            if (cell->mCell->isExterior())
            {
                ESM::Land* land =
                    MWBase::Environment::get().getWorld()->getStore().get<ESM::Land>().search(
                        cell->mCell->getGridX(),
                        cell->mCell->getGridY()
                    );
                if (land) {
                    mPhysics->addHeightField (
                        land->mLandData->mHeights,
                        cell->mCell->getGridX(),
                        cell->mCell->getGridY(),
                        0,
                        worldsize / (verts-1),
                        verts)
                    ;
                }
            }

            mRendering.configureAmbient(*cell);
            mRendering.requestMap(cell);
            mRendering.configureAmbient(*cell);
        }
    }

    void Scene::playerCellChange(MWWorld::CellStore *cell, const ESM::Position& pos, bool adjustPlayerPos)
    {
        bool hasWater = cell->mCell->mData.mFlags & ESM::Cell::HasWater;
        mPhysics->setCurrentWater(hasWater, cell->mCell->mWater);

        MWBase::World *world = MWBase::Environment::get().getWorld();
        world->getPlayer().setCell(cell);

        MWWorld::Ptr player = world->getPlayer().getPlayer();

        if (adjustPlayerPos) {
            world->moveObject(player, pos.pos[0], pos.pos[1], pos.pos[2]);

            float x = Ogre::Radian(pos.rot[0]).valueDegrees();
            float y = Ogre::Radian(pos.rot[1]).valueDegrees();
            float z = Ogre::Radian(pos.rot[2]).valueDegrees();
            world->rotateObject(player, x, y, z);
        }

        MWBase::MechanicsManager *mechMgr =
            MWBase::Environment::get().getMechanicsManager();

        mechMgr->add(player);
        mechMgr->watchActor(player);

        MWBase::Environment::get().getWindowManager()->changeCell(mCurrentCell);
    }

    void Scene::changeCell (int X, int Y, const ESM::Position& position, bool adjustPlayerPos)
    {
        Nif::NIFFile::CacheLock cachelock;
        const MWWorld::Store<ESM::GameSetting> &gmst =
            MWBase::Environment::get().getWorld()->getStore().get<ESM::GameSetting>();

        mRendering.preCellChange(mCurrentCell);

        // remove active
        MWBase::Environment::get().getMechanicsManager()->remove(MWBase::Environment::get().getWorld()->getPlayer().getPlayer());

        std::string loadingExteriorText;

        loadingExteriorText = gmst.find ("sLoadingMessage3")->getString();

        CellStoreCollection::iterator active = mActiveCells.begin();

        // get the number of cells to unload
        int numUnload = 0;
        while (active!=mActiveCells.end())
        {
            if ((*active)->mCell->isExterior())
            {
                if (std::abs (X-(*active)->mCell->getGridX())<=1 &&
                    std::abs (Y-(*active)->mCell->getGridY())<=1)
                {
                    // keep cells within the new 3x3 grid
                    ++active;
                    continue;
                }
            }
            ++active;
            ++numUnload;
        }

        int current = 0;
        active = mActiveCells.begin();
        while (active!=mActiveCells.end())
        {
            if ((*active)->mCell->isExterior())
            {
                if (std::abs (X-(*active)->mCell->getGridX())<=1 &&
                    std::abs (Y-(*active)->mCell->getGridY())<=1)
                {
                    // keep cells within the new 3x3 grid
                    ++active;
                    continue;
                }
            }
            unloadCell (active++);
            ++current;
        }

        int numLoad = 0;
        // get the number of cells to load
        for (int x=X-1; x<=X+1; ++x)
            for (int y=Y-1; y<=Y+1; ++y)
            {
                CellStoreCollection::iterator iter = mActiveCells.begin();

                while (iter!=mActiveCells.end())
                {
                    assert ((*iter)->mCell->isExterior());

                    if (x==(*iter)->mCell->getGridX() &&
                        y==(*iter)->mCell->getGridY())
                        break;

                    ++iter;
                }

                if (iter==mActiveCells.end())
                    ++numLoad;
            }

        // Load cells
        current = 0;
        for (int x=X-1; x<=X+1; ++x)
            for (int y=Y-1; y<=Y+1; ++y)
            {
                CellStoreCollection::iterator iter = mActiveCells.begin();

                while (iter!=mActiveCells.end())
                {
                    assert ((*iter)->mCell->isExterior());

                    if (x==(*iter)->mCell->getGridX() &&
                        y==(*iter)->mCell->getGridY())
                        break;

                    ++iter;
                }

                if (iter==mActiveCells.end())
                {
                    CellStore *cell = MWBase::Environment::get().getWorld()->getExterior(x, y);

                    //Loading Exterior loading text
                    MWBase::Environment::get().getWindowManager ()->setLoadingProgress (loadingExteriorText, 0, current, numLoad);

                    loadCell (cell);
                    ++current;
                }
            }

        // find current cell
        CellStoreCollection::iterator iter = mActiveCells.begin();

        while (iter!=mActiveCells.end())
        {
            assert ((*iter)->mCell->isExterior());

            if (X==(*iter)->mCell->getGridX() &&
                Y==(*iter)->mCell->getGridY())
                break;

            ++iter;
        }

        assert (iter!=mActiveCells.end());

        mCurrentCell = *iter;

        // adjust player
        playerCellChange (mCurrentCell, position, adjustPlayerPos);

        // Sky system
        MWBase::Environment::get().getWorld()->adjustSky();

        mRendering.switchToExterior();

        mCellChanged = true;

        MWBase::Environment::get().getWindowManager ()->loadingDone ();
    }

    //We need the ogre renderer and a scene node.
    Scene::Scene (MWRender::RenderingManager& rendering, PhysicsSystem *physics)
    : mCurrentCell (0), mCellChanged (false), mPhysics(physics), mRendering(rendering)
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

        const MWWorld::Store<ESM::GameSetting> &gmst =
            MWBase::Environment::get().getWorld()->getStore().get<ESM::GameSetting>();

        std::string loadingInteriorText;
        loadingInteriorText = gmst.find ("sLoadingMessage2")->getString();

        CellStore *cell = MWBase::Environment::get().getWorld()->getInterior(cellName);
        bool loadcell = (mCurrentCell == NULL);
        if(!loadcell)
            loadcell = *mCurrentCell != *cell;

        if(!loadcell)
        {
            MWBase::World *world = MWBase::Environment::get().getWorld();
            world->moveObject(world->getPlayer().getPlayer(), position.pos[0], position.pos[1], position.pos[2]);

            float x = Ogre::Radian(position.rot[0]).valueDegrees();
            float y = Ogre::Radian(position.rot[1]).valueDegrees();
            float z = Ogre::Radian(position.rot[2]).valueDegrees();
            world->rotateObject(world->getPlayer().getPlayer(), x, y, z);
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

        // Load cell.
        std::cout << "cellName: " << cell->mCell->mName << std::endl;

        //Loading Interior loading text
        MWBase::Environment::get().getWindowManager ()->setLoadingProgress (loadingInteriorText, 0, 0, 1);

        loadCell (cell);

        mCurrentCell = cell;

        // adjust fog
        mRendering.switchToInterior();
        mRendering.configureFog(*mCurrentCell);

        // adjust player
        playerCellChange (mCurrentCell, position);

        // Sky system
        MWBase::Environment::get().getWorld()->adjustSky();

        mCellChanged = true;

        MWBase::Environment::get().getWindowManager ()->loadingDone ();
    }

    void Scene::changeToExteriorCell (const ESM::Position& position)
    {
        int x = 0;
        int y = 0;

        MWBase::Environment::get().getWorld()->positionToIndex (position.pos[0], position.pos[1], x, y);

        changeCell (x, y, position, true);
    }

    Ptr::CellStore* Scene::getCurrentCell ()
    {
        return mCurrentCell;
    }

    void Scene::markCellAsUnchanged()
    {
        mCellChanged = false;
    }

    void Scene::insertCell (Ptr::CellStore &cell, bool rescale)
    {
        // Loop through all references in the cell
        insertCellRefList(mRendering, cell.mActivators, cell, *mPhysics, rescale);
        insertCellRefList(mRendering, cell.mPotions, cell, *mPhysics, rescale);
        insertCellRefList(mRendering, cell.mAppas, cell, *mPhysics, rescale);
        insertCellRefList(mRendering, cell.mArmors, cell, *mPhysics, rescale);
        insertCellRefList(mRendering, cell.mBooks, cell, *mPhysics, rescale);
        insertCellRefList(mRendering, cell.mClothes, cell, *mPhysics, rescale);
        insertCellRefList(mRendering, cell.mContainers, cell, *mPhysics, rescale);
        insertCellRefList(mRendering, cell.mCreatures, cell, *mPhysics, rescale);
        insertCellRefList(mRendering, cell.mDoors, cell, *mPhysics, rescale);
        insertCellRefList(mRendering, cell.mIngreds, cell, *mPhysics, rescale);
        insertCellRefList(mRendering, cell.mCreatureLists, cell, *mPhysics, rescale);
        insertCellRefList(mRendering, cell.mItemLists, cell, *mPhysics, rescale);
        insertCellRefList(mRendering, cell.mLights, cell, *mPhysics, rescale);
        insertCellRefList(mRendering, cell.mLockpicks, cell, *mPhysics, rescale);
        insertCellRefList(mRendering, cell.mMiscItems, cell, *mPhysics, rescale);
        insertCellRefList(mRendering, cell.mNpcs, cell, *mPhysics, rescale);
        insertCellRefList(mRendering, cell.mProbes, cell, *mPhysics, rescale);
        insertCellRefList(mRendering, cell.mRepairs, cell, *mPhysics, rescale);
        insertCellRefList(mRendering, cell.mStatics, cell, *mPhysics, rescale);
        insertCellRefList(mRendering, cell.mWeapons, cell, *mPhysics, rescale);
    }

    void Scene::addObjectToScene (const Ptr& ptr)
    {
        mRendering.addObject(ptr);
        MWWorld::Class::get(ptr).insertObject(ptr, *mPhysics);
        MWBase::Environment::get().getWorld()->rotateObject(ptr, 0, 0, 0, true);
        MWBase::Environment::get().getWorld()->scaleObject(ptr, ptr.getCellRef().mScale);
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
}
