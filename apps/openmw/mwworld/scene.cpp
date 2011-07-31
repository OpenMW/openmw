#include "scene.hpp"

#include "world.hpp"
#include "ptr.hpp"
#include "environment.hpp"
#include "class.hpp"
#include "player.hpp"

#include "refdata.hpp"
#include "globals.hpp"
#include "doingphysics.hpp"
#include "cellfunctors.hpp"
#include "environment.hpp"

#include <cmath>
#include <iostream>

#include <components/bsa/bsa_archive.hpp>
#include <components/files/collections.hpp>

#include "../mwrender/sky.hpp"
#include "../mwrender/interior.hpp"
#include "../mwrender/exterior.hpp"

#include "../mwmechanics/mechanicsmanager.hpp"

#include "../mwsound/soundmanager.hpp"

#include "ptr.hpp"
#include "environment.hpp"
#include "class.hpp"
#include "player.hpp"

#include "refdata.hpp"
#include "globals.hpp"
#include "doingphysics.hpp"
#include "cellfunctors.hpp"

namespace {

    template<typename T>
    ESMS::LiveCellRef<T, MWWorld::RefData> *searchViaHandle (const std::string& handle,
        ESMS::CellRefList<T, MWWorld::RefData>& refList)
    {
        typedef typename ESMS::CellRefList<T, MWWorld::RefData>::List::iterator iterator;

        for (iterator iter (refList.list.begin()); iter!=refList.list.end(); ++iter)
        {
            if (iter->mData.getHandle()==handle)
            {
                return &*iter;
            }
        }

        return 0;
    }
}


namespace MWWorld
{

    Scene::Scene(Environment& environment, World *world, MWRender::MWScene scene) :
        mEnvironment(environment), mWorld(world), mScene(scene)
    {
    }
    
    Ptr Scene::getPtr (const std::string& name, Ptr::CellStore& cell)
    {
        if (ESMS::LiveCellRef<ESM::Activator, RefData> *ref = cell.activators.find (name))
            return Ptr (ref, &cell);

        if (ESMS::LiveCellRef<ESM::Potion, RefData> *ref = cell.potions.find (name))
            return Ptr (ref, &cell);

        if (ESMS::LiveCellRef<ESM::Apparatus, RefData> *ref = cell.appas.find (name))
            return Ptr (ref, &cell);

        if (ESMS::LiveCellRef<ESM::Armor, RefData> *ref = cell.armors.find (name))
            return Ptr (ref, &cell);

        if (ESMS::LiveCellRef<ESM::Book, RefData> *ref = cell.books.find (name))
            return Ptr (ref, &cell);

        if (ESMS::LiveCellRef<ESM::Clothing, RefData> *ref = cell.clothes.find (name))
            return Ptr (ref, &cell);

        if (ESMS::LiveCellRef<ESM::Container, RefData> *ref = cell.containers.find (name))
            return Ptr (ref, &cell);

        if (ESMS::LiveCellRef<ESM::Creature, RefData> *ref = cell.creatures.find (name))
            return Ptr (ref, &cell);

        if (ESMS::LiveCellRef<ESM::Door, RefData> *ref = cell.doors.find (name))
            return Ptr (ref, &cell);

        if (ESMS::LiveCellRef<ESM::Ingredient, RefData> *ref = cell.ingreds.find (name))
            return Ptr (ref, &cell);

        if (ESMS::LiveCellRef<ESM::CreatureLevList, RefData> *ref = cell.creatureLists.find (name))
            return Ptr (ref, &cell);

        if (ESMS::LiveCellRef<ESM::ItemLevList, RefData> *ref = cell.itemLists.find (name))
            return Ptr (ref, &cell);

        if (ESMS::LiveCellRef<ESM::Light, RefData> *ref = cell.lights.find (name))
            return Ptr (ref, &cell);

        if (ESMS::LiveCellRef<ESM::Tool, RefData> *ref = cell.lockpicks.find (name))
            return Ptr (ref, &cell);

        if (ESMS::LiveCellRef<ESM::Miscellaneous, RefData> *ref = cell.miscItems.find (name))
            return Ptr (ref, &cell);

        if (ESMS::LiveCellRef<ESM::NPC, RefData> *ref = cell.npcs.find (name))
            return Ptr (ref, &cell);

        if (ESMS::LiveCellRef<ESM::Probe, RefData> *ref = cell.probes.find (name))
            return Ptr (ref, &cell);

        if (ESMS::LiveCellRef<ESM::Repair, RefData> *ref = cell.repairs.find (name))
            return Ptr (ref, &cell);

        if (ESMS::LiveCellRef<ESM::Static, RefData> *ref = cell.statics.find (name))
            return Ptr (ref, &cell);

        if (ESMS::LiveCellRef<ESM::Weapon, RefData> *ref = cell.weapons.find (name))
            return Ptr (ref, &cell);

        return Ptr();
    }
    
    

    Ptr Scene::getPtrViaHandle (const std::string& handle, Ptr::CellStore& cell)
    {
        if (ESMS::LiveCellRef<ESM::Activator, RefData> *ref =
            searchViaHandle (handle, cell.activators))
            return Ptr (ref, &cell);

        if (ESMS::LiveCellRef<ESM::Potion, RefData> *ref = searchViaHandle (handle, cell.potions))
            return Ptr (ref, &cell);

        if (ESMS::LiveCellRef<ESM::Apparatus, RefData> *ref = searchViaHandle (handle, cell.appas))
            return Ptr (ref, &cell);

        if (ESMS::LiveCellRef<ESM::Armor, RefData> *ref = searchViaHandle (handle, cell.armors))
            return Ptr (ref, &cell);

        if (ESMS::LiveCellRef<ESM::Book, RefData> *ref = searchViaHandle (handle, cell.books))
            return Ptr (ref, &cell);

        if (ESMS::LiveCellRef<ESM::Clothing, RefData> *ref = searchViaHandle (handle, cell.clothes))
            return Ptr (ref, &cell);

        if (ESMS::LiveCellRef<ESM::Container, RefData> *ref =
            searchViaHandle (handle, cell.containers))
            return Ptr (ref, &cell);

        if (ESMS::LiveCellRef<ESM::Creature, RefData> *ref =
            searchViaHandle (handle, cell.creatures))
            return Ptr (ref, &cell);

        if (ESMS::LiveCellRef<ESM::Door, RefData> *ref = searchViaHandle (handle, cell.doors))
            return Ptr (ref, &cell);

        if (ESMS::LiveCellRef<ESM::Ingredient, RefData> *ref =
            searchViaHandle (handle, cell.ingreds))
            return Ptr (ref, &cell);

        if (ESMS::LiveCellRef<ESM::Light, RefData> *ref = searchViaHandle (handle, cell.lights))
            return Ptr (ref, &cell);

        if (ESMS::LiveCellRef<ESM::Tool, RefData> *ref = searchViaHandle (handle, cell.lockpicks))
            return Ptr (ref, &cell);

        if (ESMS::LiveCellRef<ESM::Miscellaneous, RefData> *ref = searchViaHandle (handle, cell.miscItems))
            return Ptr (ref, &cell);

        if (ESMS::LiveCellRef<ESM::NPC, RefData> *ref = searchViaHandle (handle, cell.npcs))
            return Ptr (ref, &cell);

        if (ESMS::LiveCellRef<ESM::Probe, RefData> *ref = searchViaHandle (handle, cell.probes))
            return Ptr (ref, &cell);

        if (ESMS::LiveCellRef<ESM::Repair, RefData> *ref = searchViaHandle (handle, cell.repairs))
            return Ptr (ref, &cell);

        if (ESMS::LiveCellRef<ESM::Static, RefData> *ref = searchViaHandle (handle, cell.statics))
            return Ptr (ref, &cell);

        if (ESMS::LiveCellRef<ESM::Weapon, RefData> *ref = searchViaHandle (handle, cell.weapons))
            return Ptr (ref, &cell);

        return Ptr();
    }

    MWRender::CellRender *Scene::searchRender (Ptr::CellStore *store)
    {
        CellRenderCollection::iterator iter = mActiveCells.find (store);

        if (iter!=mActiveCells.end())
        {
            return iter->second;
        }

        return 0;
    }
    
    void Scene::unloadCell (CellRenderCollection::iterator iter)
    {
        ListHandles functor;
        iter->first->forEach<ListHandles>(functor);

        { // silence annoying g++ warning
            for (std::vector<std::string>::const_iterator iter (functor.mHandles.begin());
                iter!=functor.mHandles.end(); ++iter)
            {
                mScene.removeObject (*iter); // FIXME
            }
        }

        mEnvironment.mMechanicsManager->dropActors (iter->first);
        mEnvironment.mSoundManager->stopSound (iter->first);
        delete iter->second;
        mActiveCells.erase (iter);
    }
    
    void Scene::loadCell (Ptr::CellStore *cell, MWRender::CellRender *render)
    {
        // register local scripts
        mWorld->insertInteriorScripts (*cell); // FIXME

        // This connects the cell data with the rendering scene.
        std::pair<CellRenderCollection::iterator, bool> result =
            mActiveCells.insert (std::make_pair (cell, render));

        if (result.second)
        {
            // Load the cell and insert it into the renderer
            result.first->second->show();
        }
        
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
                    mExteriors[std::make_pair (x, y)].loadExt (x, y, mWorld->getStore(), mWorld->getEsmReader());
                    Ptr::CellStore *cell = &mExteriors[std::make_pair (x, y)];

                    loadCell (cell, new MWRender::ExteriorCellRender (*cell, mEnvironment, mScene));
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
        playerCellChange (&mExteriors[std::make_pair (X, Y)], position, adjustPlayerPos);

        // Sky system
        mWorld->adjustSky(); // FIXME

        mCellChanged = true;
        
    }
    
    Ptr Scene::getPtr (const std::string& name, bool activeOnly)
    {
        // the player is always in an active cell.
        if (name=="player")
        {
            return mWorld->getPlayer().getPlayer();
        }

        // active cells
        for (CellRenderCollection::iterator iter (mActiveCells.begin());
            iter!=mActiveCells.end(); ++iter)
        {
            Ptr ptr = getPtr (name, *iter->first);

            if (!ptr.isEmpty())
                return ptr;
        }

        if (!activeOnly)
        {
            // TODO: inactive cells
        }

        throw std::runtime_error ("unknown ID: " + name);
        
    }
    
    Ptr Scene::getPtrViaHandle (const std::string& handle)
    {
        if (mWorld->getPlayer().getPlayer().getRefData().getHandle()==handle)
            return mWorld->getPlayer().getPlayer();

        for (CellRenderCollection::iterator iter (mActiveCells.begin());
            iter!=mActiveCells.end(); ++iter)
        {
            Ptr ptr = getPtrViaHandle (handle, *iter->first);

            if (!ptr.isEmpty())
                return ptr;
        }

        throw std::runtime_error ("unknown Ogre handle: " + handle);
        
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
    
    void Scene::enable (Ptr reference)
    {
        if (!reference.getRefData().isEnabled())
        {
            reference.getRefData().enable();

            if (MWRender::CellRender *render = searchRender (reference.getCell()))
            {
                render->enable (reference.getRefData().getHandle());

                if (mActiveCells.find (reference.getCell())!=mActiveCells.end())
                {
                    Class::get (reference).enable (reference, mEnvironment); //FIXME
                }
            }
        }
    }
    
    void Scene::disable (Ptr reference)
    {
        if (reference.getRefData().isEnabled())
        {
            reference.getRefData().disable();

            if (MWRender::CellRender *render = searchRender (reference.getCell()))
            {
                render->disable (reference.getRefData().getHandle());

                if (mActiveCells.find (reference.getCell())!=mActiveCells.end())
                {
                    Class::get (reference).disable (reference, mEnvironment);
                    mEnvironment.mSoundManager->stopSound3D (reference);
                }
            }
        }
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
        mInteriors[cellName].loadInt (cellName, mWorld->getStore(), mWorld->getEsmReader());
        Ptr::CellStore *cell = &mInteriors[cellName];

        loadCell (cell, new MWRender::InteriorCellRender (*cell, mEnvironment, mScene));

        // adjust player
        mCurrentCell = cell;
        playerCellChange (cell, position, true); // FIXME

        // Sky system
        mWorld->adjustSky(); // FIXME

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
    
    const ESM::Cell *Scene::getExterior (const std::string& cellName) const
    {
        // first try named cells
        if (const ESM::Cell *cell = mWorld->getStore().cells.searchExtByName (cellName))
            return cell;

        // didn't work -> now check for regions
        std::string cellName2 = ESMS::RecListT<ESM::Region>::toLower (cellName);

        for (ESMS::RecListT<ESM::Region>::MapType::const_iterator iter (mWorld->getStore().regions.list.begin());
            iter!=mWorld->getStore().regions.list.end(); ++iter)
        {
            if (ESMS::RecListT<ESM::Region>::toLower (iter->second.name)==cellName2)
            {
                if (const ESM::Cell *cell = mWorld->getStore().cells.searchExtByRegion (iter->first))
                    return cell;

                break;
            }
        }

        return 0;
    }
    
    void Scene::deleteObject (Ptr ptr)
    {
        if (ptr.getRefData().getCount()>0)
        {
            ptr.getRefData().setCount (0);

            if (MWRender::CellRender *render = searchRender (ptr.getCell()))
            {
                if (mActiveCells.find (ptr.getCell())!=mActiveCells.end())
                {
                    Class::get (ptr).disable (ptr, mEnvironment);
                    mEnvironment.mSoundManager->stopSound3D (ptr);

                    if (!DoingPhysics::isDoingPhysics())
                        mScene.removeObject (ptr.getRefData().getHandle());
                }

                render->deleteObject (ptr.getRefData().getHandle());
                ptr.getRefData().setHandle ("");
            }
        }
    }
    
    void Scene::moveObject (Ptr ptr, float x, float y, float z)
    {
        ptr.getCellRef().pos.pos[0] = x;
        ptr.getCellRef().pos.pos[1] = y;
        ptr.getCellRef().pos.pos[2] = z;

        if (ptr==mWorld->getPlayer().getPlayer())
        {
            if (mCurrentCell)
            {
                if (!(mCurrentCell->cell->data.flags & ESM::Cell::Interior))
                {
                    // exterior -> adjust loaded cells
                    int cellX = 0;
                    int cellY = 0;

                    mWorld->positionToIndex (x, y, cellX, cellY);

                    if (mCurrentCell->cell->data.gridX!=cellX || mCurrentCell->cell->data.gridY!=cellY)
                    {
                        changeCell (cellX, cellY, mWorld->getPlayer().getPlayer().getCellRef().pos, false);
                    }

                }
            }
        }

        mScene.moveObject (ptr.getRefData().getHandle(), Ogre::Vector3 (x, y, z),
            !DoingPhysics::isDoingPhysics());

        // TODO cell change for non-player ref
    }

}
