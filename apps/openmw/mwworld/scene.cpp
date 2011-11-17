#include "scene.hpp"
#include "world.hpp"


#include "../mwmechanics/mechanicsmanager.hpp"

#include "../mwsound/soundmanager.hpp"

#include "ptr.hpp"
#include "environment.hpp"
#include "player.hpp"
#include "class.hpp"

#include "cellfunctors.hpp"

namespace {



template<typename T>
void insertCellRefList (T& cellRefList, ESMS::CellStore<MWWorld::RefData> &cell)
{
    if (!cellRefList.list.empty())
    {
        //const MWWorld::Class& class_ = MWWorld::Class::get (MWWorld::Ptr (&*cellRefList.list.begin(), &cell));

        for (typename T::List::iterator it = cellRefList.list.begin();
            it != cellRefList.list.end(); it++)
        {
            if (it->mData.getCount() || it->mData.isEnabled())
            {
                MWWorld::Ptr ptr (&*it, &cell);
                /* TODO: call
                    * RenderingManager.insertObject
                    * class_.insertObjectPhysic
                    * class_.insertObjectMechanics
                */
            }
        }
   }
}
template<typename T>
void insertCellRefList(MWRender::RenderingManager& rendering, MWWorld::Environment& environment,
    T& cellRefList, ESMS::CellStore<MWWorld::RefData> &cell, MWWorld::PhysicsSystem& physics)
{
    if (!cellRefList.list.empty())
    {
        const MWWorld::Class& class_ =
            MWWorld::Class::get (MWWorld::Ptr (&*cellRefList.list.begin(), &cell));

        for (typename T::List::iterator it = cellRefList.list.begin();
            it != cellRefList.list.end(); it++)
        {
            if (it->mData.getCount() || it->mData.isEnabled())
            {
                MWWorld::Ptr ptr (&*it, &cell);

                try
                {
                 
                    rendering.addObject(ptr);
                    class_.insertObject(ptr, physics, environment);
                    class_.enable (ptr, environment);
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

    void Scene::unloadCell (CellStoreCollection::iterator iter)
    {
        ListHandles functor;
        Ptr::CellStore* cellstore = *iter;
       
        cellstore->forEach<ListHandles>(functor);

        { // silence annoying g++ warning
            for (std::vector<std::string>::const_iterator iter (functor.mHandles.begin());
                iter!=functor.mHandles.end(); ++iter)
                mPhysics->removeObject (*iter);
        }

        mWorld->getLocalScripts().clearCell (cellstore);

        mEnvironment.mMechanicsManager->dropActors (cellstore);
        mEnvironment.mSoundManager->stopSound (cellstore);
        //delete iter->second;
        mActiveCells.erase (iter);
    }

    void Scene::loadCell (Ptr::CellStore *cell)
    {
        // register local scripts
        mWorld->getLocalScripts().addCell (cell);

        // This connects the cell data with the rendering scene.
            mActiveCells.insert(cell);
        
       mRendering.getObjects().buildStaticGeometry(*cell);
       insertCell(*cell, mEnvironment);
         
    }

    void Scene::playerCellChange (Ptr::CellStore *cell, const ESM::Position& position,
        bool adjustPlayerPos)
    {
        if (adjustPlayerPos)
            mWorld->getPlayer().setPos (position.pos[0], position.pos[1], position.pos[2]);

        mWorld->getPlayer().setCell (cell);
        // TODO orientation
        mEnvironment.mMechanicsManager->addActor (mWorld->getPlayer().getPlayer());
        mEnvironment.mMechanicsManager->watchActor (mWorld->getPlayer().getPlayer());
    }

    void Scene::changeCell (int X, int Y, const ESM::Position& position, bool adjustPlayerPos)
    {
        // remove active
        mEnvironment.mMechanicsManager->removeActor (mWorld->getPlayer().getPlayer());

        CellStoreCollection::iterator active = mActiveCells.begin();
         Ptr::CellStore* cellstore = *active;
        
        while (active!=mActiveCells.end())
        {
            cellstore = *active;
            if (!(cellstore->cell->data.flags & ESM::Cell::Interior))
            {
                if (std::abs (X-cellstore->cell->data.gridX)<=1 &&
                    std::abs (Y-cellstore->cell->data.gridY)<=1)
                {
                    // keep cells within the new 3x3 grid
                    ++active;
                    continue;
                }
            }

            unloadCell (active++);
        }

        // Load cells
        for (int x=X-1; x<=X+1; ++x)
            for (int y=Y-1; y<=Y+1; ++y)
            {
                CellStoreCollection::iterator iter = mActiveCells.begin();
               

                while (iter!=mActiveCells.end())
                {
                    cellstore = *iter;
                    assert (!(cellstore->cell->data.flags & ESM::Cell::Interior));

                    if (x==cellstore->cell->data.gridX &&
                        y==cellstore->cell->data.gridY)
                        break;

                    ++iter;
                }

                if (iter==mActiveCells.end())
                {
                    Ptr::CellStore *cell = mWorld->getExterior(x, y);

                    loadCell (cell);
                }
            }

        // find current cell
        CellStoreCollection::iterator iter = mActiveCells.begin();
      

        while (iter!=mActiveCells.end())
        {
            cellstore = *iter;
            assert (!(iter->first->cell->data.flags & ESM::Cell::Interior));

            if (X==cellstore->cell->data.gridX &&
                Y==cellstore->cell->data.gridY)
                break;

            ++iter;
        }

        assert (iter!=mActiveCells.end());

        mCurrentCell = cellstore;

        // adjust player
        playerCellChange (mWorld->getExterior(X, Y), position, adjustPlayerPos);

        // Sky system
        mWorld->adjustSky();

        mCellChanged = true;
    }

	//We need the ogre renderer and a scene node.
    Scene::Scene (Environment& environment, World *world, MWRender::RenderingManager& rendering, PhysicsSystem *physics)
    : mRendering(rendering), mCurrentCell (0),
      mCellChanged (false), mEnvironment (environment), mWorld(world), mPhysics(physics)
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
        // remove active
        CellStoreCollection::iterator active = mActiveCells.begin();

        while (active!=mActiveCells.end())
        {
            unloadCell (active++);
        }

        // Load cell.
        std::cout << "cellName:" << cellName << std::endl;
        Ptr::CellStore *cell = mWorld->getInterior(cellName);

        loadCell (cell);

        // adjust player
        mCurrentCell = cell;
        playerCellChange (cell, position);

        // Sky system
        mWorld->adjustSky();

        mCellChanged = true;
        //currentRegion->name = "";
    }

    void Scene::changeToExteriorCell (const ESM::Position& position)
    {
        int x = 0;
        int y = 0;

        mWorld->positionToIndex (position.pos[0], position.pos[1], x, y);

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

/*#include <cassert>
#include <iostream>
#include <exception>

#include "../mwworld/class.hpp"
#include "../mwworld/ptr.hpp"*/

void Scene::insertCell(ESMS::CellStore<MWWorld::RefData> &cell)
{
  // Loop through all references in the cell
  insertCellRefList (cell.activators, cell);
  insertCellRefList (cell.potions, cell);
  insertCellRefList (cell.appas, cell);
  insertCellRefList (cell.armors, cell);
  insertCellRefList (cell.books, cell);
  insertCellRefList (cell.clothes, cell);
  insertCellRefList (cell.containers, cell);
  insertCellRefList (cell.creatures, cell);
  insertCellRefList (cell.doors, cell);
  insertCellRefList (cell.ingreds, cell);
  insertCellRefList (cell.creatureLists, cell);
  insertCellRefList (cell.itemLists, cell);
  insertCellRefList (cell.lights, cell);
  insertCellRefList (cell.lockpicks, cell);
  insertCellRefList (cell.miscItems, cell);
  insertCellRefList (cell.npcs, cell);
  insertCellRefList (cell.probes, cell);
  insertCellRefList (cell.repairs, cell);
  insertCellRefList (cell.statics, cell);
  insertCellRefList(cell.weapons, cell);
}

void Scene::insertCell(ESMS::CellStore<MWWorld::RefData> &cell,
    MWWorld::Environment& environment)
{
  // Loop through all references in the cell
  insertCellRefList(mRendering, environment, cell.activators, cell, *mPhysics);
  insertCellRefList(mRendering, environment, cell.potions, cell, *mPhysics);
  insertCellRefList(mRendering, environment, cell.appas, cell, *mPhysics);
  insertCellRefList(mRendering, environment, cell.armors, cell, *mPhysics);
  insertCellRefList(mRendering, environment, cell.books, cell, *mPhysics);
  insertCellRefList(mRendering, environment, cell.clothes, cell, *mPhysics);
  insertCellRefList(mRendering, environment, cell.containers, cell, *mPhysics);
  insertCellRefList(mRendering, environment, cell.creatures, cell, *mPhysics);
  insertCellRefList(mRendering, environment, cell.doors, cell, *mPhysics);
  insertCellRefList(mRendering, environment, cell.ingreds, cell, *mPhysics);
  insertCellRefList(mRendering, environment, cell.creatureLists, cell, *mPhysics);
  insertCellRefList(mRendering, environment, cell.itemLists, cell, *mPhysics);
  insertCellRefList(mRendering, environment, cell.lights, cell, *mPhysics);
  insertCellRefList(mRendering, environment, cell.lockpicks, cell, *mPhysics);
  insertCellRefList(mRendering, environment, cell.miscItems, cell, *mPhysics);
  insertCellRefList(mRendering, environment, cell.npcs, cell, *mPhysics);
  insertCellRefList(mRendering, environment, cell.probes, cell, *mPhysics);
  insertCellRefList(mRendering, environment, cell.repairs, cell, *mPhysics);
  insertCellRefList(mRendering, environment, cell.statics, cell, *mPhysics);
  insertCellRefList(mRendering, environment, cell.weapons, cell, *mPhysics);
}


}
