#include "scene.hpp"
#include "world.hpp"


#include "../mwmechanics/mechanicsmanager.hpp"

#include "../mwsound/soundmanager.hpp"

#include "../mwgui/window_manager.hpp"

#include "ptr.hpp"
#include "environment.hpp"
#include "player.hpp"
#include "class.hpp"

#include "cellfunctors.hpp"

namespace {

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
            for (std::vector<Ogre::SceneNode*>::const_iterator iter (functor.mHandles.begin());
                iter!=functor.mHandles.end(); ++iter){
                 Ogre::SceneNode* node = *iter;
                mPhysics->removeObject (node->getName());
            }
        }
		mRendering.removeCell(*iter);
		//mPhysics->removeObject("Unnamed_43");

        mWorld->getLocalScripts().clearCell (*iter);
        mEnvironment.mMechanicsManager->dropActors (*iter);
        mEnvironment.mSoundManager->stopSound (*iter);
		mActiveCells.erase(*iter);
        
        
    }

    void Scene::loadCell (Ptr::CellStore *cell)
    {
        // register local scripts
        mWorld->getLocalScripts().addCell (cell);



        std::pair<CellStoreCollection::iterator, bool> result =
            mActiveCells.insert(cell);
       if(result.second){
              insertCell(*cell, mEnvironment);
               mRendering.cellAdded (cell);
               mRendering.configureAmbient(*cell);
               mRendering.requestMap(cell);
               mRendering.configureAmbient(*cell);
        }


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

        // set map window cell name
        if (!(mCurrentCell->cell->data.flags & ESM::Cell::Interior))
        {
            if (mCurrentCell->cell->name != "")
                mEnvironment.mWindowManager->setCellName( mCurrentCell->cell->name );
            else
                mEnvironment.mWindowManager->setCellName( mCurrentCell->cell->region );
        }
        else
            mEnvironment.mWindowManager->setCellName( mCurrentCell->cell->name );
    }

    void Scene::changeCell (int X, int Y, const ESM::Position& position, bool adjustPlayerPos)
    {
        mRendering.preCellChange(mCurrentCell);

        // remove active
        mEnvironment.mMechanicsManager->removeActor (mWorld->getPlayer().getPlayer());

        CellStoreCollection::iterator active = mActiveCells.begin();

        while (active!=mActiveCells.end())
        {
            if (!((*active)->cell->data.flags & ESM::Cell::Interior))
            {
                if (std::abs (X-(*active)->cell->data.gridX)<=1 &&
                    std::abs (Y-(*active)->cell->data.gridY)<=1)
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
                    assert (!((*iter)->cell->data.flags & ESM::Cell::Interior));

                    if (x==(*iter)->cell->data.gridX &&
                        y==(*iter)->cell->data.gridY)
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
            assert (!((*iter)->cell->data.flags & ESM::Cell::Interior));

            if (X==(*iter)->cell->data.gridX &&
                Y==(*iter)->cell->data.gridY)
                break;

            ++iter;
        }

        assert (iter!=mActiveCells.end());

        mCurrentCell = *iter;

        // adjust player
        playerCellChange (mWorld->getExterior(X, Y), position, adjustPlayerPos);

        // Sky system
        mWorld->adjustSky();

        mCellChanged = true;
    }

    //We need the ogre renderer and a scene node.
    Scene::Scene (Environment& environment, World *world, MWRender::RenderingManager& rendering, PhysicsSystem *physics)
    : mCurrentCell (0), mCellChanged (false), mEnvironment (environment), mWorld(world),
      mPhysics(physics), mRendering(rendering)
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
        
        // adjust fog
        mRendering.configureFog(*cell);

        // Sky system
        mWorld->adjustSky();

        mCellChanged = true;
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
