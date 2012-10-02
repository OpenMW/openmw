#include "scene.hpp"

#include <components/esm_store/store.hpp>

#include "../mwbase/environment.hpp"
#include "../mwbase/world.hpp" /// FIXME
#include "../mwbase/soundmanager.hpp"
#include "../mwbase/mechanicsmanager.hpp"
#include "../mwbase/windowmanager.hpp"

#include "player.hpp"
#include "localscripts.hpp"

#include "cellfunctors.hpp"

namespace
{

    template<typename T>
    void insertCellRefList(MWRender::RenderingManager& rendering,
        T& cellRefList, MWWorld::CellStore &cell, MWWorld::PhysicsSystem& physics)
    {
        if (!cellRefList.list.empty())
        {
            const MWWorld::Class& class_ =
                MWWorld::Class::get (MWWorld::Ptr (&*cellRefList.list.begin(), &cell));

            int numRefs = cellRefList.list.size();
            int current = 0;
            for (typename T::List::iterator it = cellRefList.list.begin();
                it != cellRefList.list.end(); it++)
            {
                MWBase::Environment::get().getWindowManager ()->setLoadingProgress ("Loading cells", 1, current, numRefs);
                ++current;

                if (it->mData.getCount() || it->mData.isEnabled())
                {
                    MWWorld::Ptr ptr (&*it, &cell);

                    try
                    {
                        rendering.addObject(ptr);
                        class_.insertObject(ptr, physics);
                    }
                    catch (const std::exception& e)
                    {
                        std::string error ("error during rendering: ");
                        std::cerr << error + e.what() << std::endl;
                    }
                }
            }
        }
        else
        {
            MWBase::Environment::get().getWindowManager ()->setLoadingProgress ("Loading cells", 1, 0, 1);
        }
    }

}


namespace MWWorld
{

    void Scene::update (float duration){
        mRendering.update (duration);
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

            if (!((*iter)->cell->mData.mFlags & ESM::Cell::Interior))
            {
                ESM::Land* land = MWBase::Environment::get().getWorld()->getStore().lands.search((*iter)->cell->mData.mX,(*iter)->cell->mData.mY);
                if (land)
                    mPhysics->removeHeightField( (*iter)->cell->mData.mX, (*iter)->cell->mData.mY );
            }
        }

		mRendering.removeCell(*iter);
		//mPhysics->removeObject("Unnamed_43");

        MWBase::Environment::get().getWorld()->getLocalScripts().clearCell (*iter);
        MWBase::Environment::get().getMechanicsManager()->dropActors (*iter);
        MWBase::Environment::get().getSoundManager()->stopSound (*iter);
		mActiveCells.erase(*iter);



    }

    void Scene::loadCell (Ptr::CellStore *cell)
    {
        // register local scripts
        MWBase::Environment::get().getWorld()->getLocalScripts().addCell (cell);



        std::pair<CellStoreCollection::iterator, bool> result =
            mActiveCells.insert(cell);

        if(result.second)
        {
            insertCell(*cell);
            mRendering.cellAdded (cell);

            float verts = ESM::Land::LAND_SIZE;
            float worldsize = ESM::Land::REAL_SIZE;

            if (!(cell->cell->mData.mFlags & ESM::Cell::Interior))
            {
                ESM::Land* land = MWBase::Environment::get().getWorld()->getStore().lands.search(cell->cell->mData.mX,cell->cell->mData.mY);
                if (land)
                    mPhysics->addHeightField (land->mLandData->mHeights,
                        cell->cell->mData.mX, cell->cell->mData.mY,
                        0, ( worldsize/(verts-1) ), verts);
            }

            mRendering.configureAmbient(*cell);
            mRendering.requestMap(cell);
            mRendering.configureAmbient(*cell);

        }

    }

    void
    Scene::playerCellChange(
        MWWorld::CellStore *cell,
        const ESM::Position& pos,
        bool adjustPlayerPos)
    {
        bool hasWater = cell->cell->mData.mFlags & cell->cell->HasWater;
        mPhysics->setCurrentWater(hasWater, cell->cell->mWater);

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

        mechMgr->addActor(player);
        mechMgr->watchActor(player);

        MWBase::Environment::get().getWindowManager()->changeCell(mCurrentCell);
    }

    void Scene::changeCell (int X, int Y, const ESM::Position& position, bool adjustPlayerPos)
    {
        mRendering.preCellChange(mCurrentCell);

        // remove active
        MWBase::Environment::get().getMechanicsManager()->removeActor (MWBase::Environment::get().getWorld()->getPlayer().getPlayer());

        CellStoreCollection::iterator active = mActiveCells.begin();

        // get the number of cells to unload
        int numUnload = 0;
        while (active!=mActiveCells.end())
        {
            if (!((*active)->cell->mData.mFlags & ESM::Cell::Interior))
            {
                if (std::abs (X-(*active)->cell->mData.mX)<=1 &&
                    std::abs (Y-(*active)->cell->mData.mY)<=1)
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
            if (!((*active)->cell->mData.mFlags & ESM::Cell::Interior))
            {
                if (std::abs (X-(*active)->cell->mData.mX)<=1 &&
                    std::abs (Y-(*active)->cell->mData.mY)<=1)
                {
                    // keep cells within the new 3x3 grid
                    ++active;
                    continue;
                }
            }

            MWBase::Environment::get().getWindowManager ()->setLoadingProgress ("Unloading cells", 0, current, numUnload);
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
                    assert (!((*iter)->cell->mData.mFlags & ESM::Cell::Interior));

                    if (x==(*iter)->cell->mData.mX &&
                        y==(*iter)->cell->mData.mY)
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
                    assert (!((*iter)->cell->mData.mFlags & ESM::Cell::Interior));

                    if (x==(*iter)->cell->mData.mX &&
                        y==(*iter)->cell->mData.mY)
                        break;

                    ++iter;
                }

                if (iter==mActiveCells.end())
                {
                    CellStore *cell = MWBase::Environment::get().getWorld()->getExterior(x, y);

                    MWBase::Environment::get().getWindowManager ()->setLoadingProgress ("Loading cells", 0, current, numLoad);
                    loadCell (cell);
                    ++current;
                }
            }

        // find current cell
        CellStoreCollection::iterator iter = mActiveCells.begin();

        while (iter!=mActiveCells.end())
        {
            assert (!((*iter)->cell->mData.mFlags & ESM::Cell::Interior));

            if (X==(*iter)->cell->mData.mX &&
                Y==(*iter)->cell->mData.mY)
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
        std::cout << "Changing to interior\n";

        CellStore *cell = MWBase::Environment::get().getWorld()->getInterior(cellName);

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
            MWBase::Environment::get().getWindowManager ()->setLoadingProgress ("Unloading cells", 0, current, numUnload);

            unloadCell (active++);
            ++current;
        }

        // Load cell.
        std::cout << "cellName:" << cellName << std::endl;


        MWBase::Environment::get().getWindowManager ()->setLoadingProgress ("Loading cells", 0, 0, 1);
        loadCell (cell);

        // adjust player
        mCurrentCell = cell;
        playerCellChange (cell, position);

        // adjust fog
        mRendering.switchToInterior();
        mRendering.configureFog(*cell);

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

    void Scene::insertCell (Ptr::CellStore &cell)
    {
        // Loop through all references in the cell
        insertCellRefList(mRendering, cell.activators, cell, *mPhysics);
        insertCellRefList(mRendering, cell.potions, cell, *mPhysics);
        insertCellRefList(mRendering, cell.appas, cell, *mPhysics);
        insertCellRefList(mRendering, cell.armors, cell, *mPhysics);
        insertCellRefList(mRendering, cell.books, cell, *mPhysics);
        insertCellRefList(mRendering, cell.clothes, cell, *mPhysics);
        insertCellRefList(mRendering, cell.containers, cell, *mPhysics);
        insertCellRefList(mRendering, cell.creatures, cell, *mPhysics);
        insertCellRefList(mRendering, cell.doors, cell, *mPhysics);
        insertCellRefList(mRendering, cell.ingreds, cell, *mPhysics);
        insertCellRefList(mRendering, cell.creatureLists, cell, *mPhysics);
        insertCellRefList(mRendering, cell.itemLists, cell, *mPhysics);
        insertCellRefList(mRendering, cell.lights, cell, *mPhysics);
        insertCellRefList(mRendering, cell.lockpicks, cell, *mPhysics);
        insertCellRefList(mRendering, cell.miscItems, cell, *mPhysics);
        insertCellRefList(mRendering, cell.npcs, cell, *mPhysics);
        insertCellRefList(mRendering, cell.probes, cell, *mPhysics);
        insertCellRefList(mRendering, cell.repairs, cell, *mPhysics);
        insertCellRefList(mRendering, cell.statics, cell, *mPhysics);
        insertCellRefList(mRendering, cell.weapons, cell, *mPhysics);
    }

    void Scene::addObjectToScene (const Ptr& ptr)
    {
        mRendering.addObject(ptr);
        MWWorld::Class::get(ptr).insertObject(ptr, *mPhysics);
    }

    void Scene::removeObjectFromScene (const Ptr& ptr)
    {
        MWBase::Environment::get().getMechanicsManager()->removeActor (ptr);
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
