#include "scene.hpp"
#include "world.hpp"

#include "../mwrender/interior.hpp"
#include "../mwrender/exterior.hpp"

#include "../mwmechanics/mechanicsmanager.hpp"

#include "../mwsound/soundmanager.hpp"

#include "ptr.hpp"
#include "environment.hpp"
#include "player.hpp"
#include "class.hpp"

#include "doingphysics.hpp"
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

}


namespace MWWorld
{

    void Scene::unloadCell (CellRenderCollection::iterator iter)
    {
        ListHandles functor;
        iter->first->forEach<ListHandles>(functor);

        { // silence annoying g++ warning
            for (std::vector<std::string>::const_iterator iter (functor.mHandles.begin());
                iter!=functor.mHandles.end(); ++iter)
                mPhysics->removeObject (*iter);
        }

        mWorld->removeScripts (iter->first);

        mEnvironment.mMechanicsManager->dropActors (iter->first); // FIXME: gehört in world?
        mEnvironment.mSoundManager->stopSound (iter->first); // FIXME: same
        delete iter->second;
        mActiveCells.erase (iter);
    }

    void Scene::loadCell (Ptr::CellStore *cell, MWRender::CellRender *render)
    {
        // register local scripts
        mWorld->insertInteriorScripts (*cell);

        // This connects the cell data with the rendering scene.
        std::pair<CellRenderCollection::iterator, bool> result =
            mActiveCells.insert (std::make_pair (cell, render));

        if (result.second)
        {
            // Load the cell and insert it into the renderer
            result.first->second->show();
        }
    }

    void Scene::playerCellChange (Ptr::CellStore *cell, const ESM::Position& position,
        bool adjustPlayerPos)
    {
        if (adjustPlayerPos)
            mWorld->getPlayer().setPos (position.pos[0], position.pos[1], position.pos[2], false);

        mWorld->getPlayer().setCell (cell);
        // TODO orientation
        mEnvironment.mMechanicsManager->addActor (mWorld->getPlayer().getPlayer());
        mEnvironment.mMechanicsManager->watchActor (mWorld->getPlayer().getPlayer());
    }

    void Scene::changeCell (int X, int Y, const ESM::Position& position, bool adjustPlayerPos)
    {
        SuppressDoingPhysics scopeGuard;
        // remove active
        mEnvironment.mMechanicsManager->removeActor (mWorld->getPlayer().getPlayer());

        CellRenderCollection::iterator active = mActiveCells.begin();

        while (active!=mActiveCells.end())
        {
            if (!(active->first->cell->data.flags & ESM::Cell::Interior))
            {
                if (std::abs (X-active->first->cell->data.gridX)<=1 &&
                    std::abs (Y-active->first->cell->data.gridY)<=1)
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
                CellRenderCollection::iterator iter = mActiveCells.begin();

                while (iter!=mActiveCells.end())
                {
                    assert (!(iter->first->cell->data.flags & ESM::Cell::Interior));

                    if (x==iter->first->cell->data.gridX &&
                        y==iter->first->cell->data.gridY)
                        break;

                    ++iter;
                }

                if (iter==mActiveCells.end())
                {
                    mWorld->getExterior(x, y)->loadExt (x, y, mWorld->getStore(), mWorld->getEsmReader());
                    Ptr::CellStore *cell = mWorld->getExterior(x, y);

                    loadCell (cell, new MWRender::ExteriorCellRender (*cell, mEnvironment, mScene, mPhysics));
                }
            }

        // find current cell
        CellRenderCollection::iterator iter = mActiveCells.begin();

        while (iter!=mActiveCells.end())
        {
            assert (!(iter->first->cell->data.flags & ESM::Cell::Interior));

            if (X==iter->first->cell->data.gridX &&
                Y==iter->first->cell->data.gridY)
                break;

            ++iter;
        }

        assert (iter!=mActiveCells.end());

        mCurrentCell = iter->first;

        // adjust player
        playerCellChange (mWorld->getExterior(X, Y), position, adjustPlayerPos);

        // Sky system
        mWorld->adjustSky();

        mCellChanged = true;
    }

    Scene::Scene (Environment& environment, World *world, MWRender::MWScene& scene, PhysicsSystem *physics)
    : mScene (scene), mCurrentCell (0),
      mCellChanged (false), mEnvironment (environment), mWorld(world), mPhysics(physics)
    {
    }

    Scene::~Scene()
    {
        for (CellRenderCollection::iterator iter (mActiveCells.begin());
            iter!=mActiveCells.end(); ++iter)
            delete iter->second;
    }

    bool Scene::hasCellChanged() const
    {
        return mCellChanged;
    }

    std::map<Ptr::CellStore *, MWRender::CellRender *> Scene::getActiveCells ()
    {
        return mActiveCells;
    }

    void Scene::changeToInteriorCell (const std::string& cellName, const ESM::Position& position)
    {
        SuppressDoingPhysics scopeGuard;

        // remove active
        CellRenderCollection::iterator active = mActiveCells.begin();

        while (active!=mActiveCells.end())
        {
            unloadCell (active++);
        }

        // Load cell.
        std::cout << "cellName:" << cellName << std::endl;
        mWorld->getInterior(cellName)->loadInt (cellName, mWorld->getStore(), mWorld->getEsmReader());
        Ptr::CellStore *cell = mWorld->getInterior(cellName);

        loadCell (cell, new MWRender::InteriorCellRender (*cell, mEnvironment, mScene, mPhysics));

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
  insertCellRefList (cell.weapons, cell);
}

}
