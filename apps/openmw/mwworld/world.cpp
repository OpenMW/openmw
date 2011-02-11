
#include "world.hpp"

#include <cmath>
#include <iostream>

#include <components/bsa/bsa_archive.hpp>

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

namespace
{
    template<typename T>
    void listCellScripts (const ESMS::ESMStore& store,
        ESMS::CellRefList<T, MWWorld::RefData>& cellRefList, MWWorld::World::ScriptList& scriptList,
        MWWorld::Ptr::CellStore *cell)
    {
        for (typename ESMS::CellRefList<T, MWWorld::RefData>::List::iterator iter (
            cellRefList.list.begin());
            iter!=cellRefList.list.end(); ++iter)
        {
            if (!iter->base->script.empty() && iter->mData.getCount())
            {
                if (const ESM::Script *script = store.scripts.find (iter->base->script))
                {
                    iter->mData.setLocals (*script);

                    scriptList.push_back (
                        std::make_pair (iter->base->script, MWWorld::Ptr (&*iter, cell)));
                }
            }
        }
    }

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

    void World::insertInteriorScripts (ESMS::CellStore<RefData>& cell)
    {
        listCellScripts (mStore, cell.activators, mLocalScripts, &cell);
        listCellScripts (mStore, cell.potions, mLocalScripts, &cell);
        listCellScripts (mStore, cell.appas, mLocalScripts, &cell);
        listCellScripts (mStore, cell.armors, mLocalScripts, &cell);
        listCellScripts (mStore, cell.books, mLocalScripts, &cell);
        listCellScripts (mStore, cell.clothes, mLocalScripts, &cell);
        listCellScripts (mStore, cell.containers, mLocalScripts, &cell);
        listCellScripts (mStore, cell.creatures, mLocalScripts, &cell);
        listCellScripts (mStore, cell.doors, mLocalScripts, &cell);
        listCellScripts (mStore, cell.ingreds, mLocalScripts, &cell);
        listCellScripts (mStore, cell.lights, mLocalScripts, &cell);
        listCellScripts (mStore, cell.lockpicks, mLocalScripts, &cell);
        listCellScripts (mStore, cell.miscItems, mLocalScripts, &cell);
        listCellScripts (mStore, cell.npcs, mLocalScripts, &cell);
        listCellScripts (mStore, cell.probes, mLocalScripts, &cell);
        listCellScripts (mStore, cell.repairs, mLocalScripts, &cell);
        listCellScripts (mStore, cell.weapons, mLocalScripts, &cell);
    }

    Ptr World::getPtr (const std::string& name, Ptr::CellStore& cell)
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

        if (ESMS::LiveCellRef<ESM::Misc, RefData> *ref = cell.miscItems.find (name))
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

    Ptr World::getPtrViaHandle (const std::string& handle, Ptr::CellStore& cell)
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

        if (ESMS::LiveCellRef<ESM::Misc, RefData> *ref = searchViaHandle (handle, cell.miscItems))
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

    MWRender::CellRender *World::searchRender (Ptr::CellStore *store)
    {
        CellRenderCollection::iterator iter = mActiveCells.find (store);

        if (iter!=mActiveCells.end())
        {
            return iter->second;
        }
        else
        {
            iter = mBufferedCells.find (store);
            if (iter!=mBufferedCells.end())
                return iter->second;
        }

        return 0;
    }

    int World::getDaysPerMonth (int month) const
    {
        switch (month)
        {
            case 0: return 31;
            case 1: return 28;
            case 2: return 31;
            case 3: return 30;
            case 4: return 31;
            case 5: return 30;
            case 6: return 31;
            case 7: return 31;
            case 8: return 30;
            case 9: return 31;
            case 10: return 30;
            case 11: return 31;
        }

        throw std::runtime_error ("month out of range");
    }

    void World::removeScripts (Ptr::CellStore *cell)
    {
        ScriptList::iterator iter = mLocalScripts.begin();

        while (iter!=mLocalScripts.end())
        {
            if (iter->second.getCell()==cell)
                mLocalScripts.erase (iter++);
            else
                ++iter;
        }
    }

    void World::unloadCell (CellRenderCollection::iterator iter)
    {
        ListHandles functor;
        iter->first->forEach (functor);

        { // silence annoying g++ warning
            for (std::vector<std::string>::const_iterator iter (functor.mHandles.begin());
                iter!=functor.mHandles.end(); ++iter)
                mScene.removeObject (*iter);
        }

        removeScripts (iter->first);
        mEnvironment.mMechanicsManager->dropActors (iter->first);
        iter->second->destroy();
        mEnvironment.mSoundManager->stopSound (iter->first);
        delete iter->second;
        mActiveCells.erase (iter);
    }

    void World::loadCell (Ptr::CellStore *cell, MWRender::CellRender *render)
    {
        // register local scripts
        insertInteriorScripts (*cell);

        // This connects the cell data with the rendering scene.
        std::pair<CellRenderCollection::iterator, bool> result =
            mActiveCells.insert (std::make_pair (cell, render));

        if (result.second)
        {
            // Load the cell and insert it into the renderer
            result.first->second->show();
        }
    }

    void World::playerCellChange (Ptr::CellStore *cell, const ESM::Position& position,
        bool adjustPlayerPos)
    {
        if (adjustPlayerPos)
            mPlayer->setPos (position.pos[0], position.pos[1], position.pos[2], true);

        mPlayer->setCell (cell);
        // TODO orientation

        mEnvironment.mMechanicsManager->addActor (mPlayer->getPlayer());
        mEnvironment.mMechanicsManager->watchActor (mPlayer->getPlayer());
    }


    void World::adjustSky()
    {
        if (mSky)
        {
            toggleSky();
            // TODO set weather
            toggleSky();
        }
    }

    void World::changeCell (int X, int Y, const ESM::Position& position, bool adjustPlayerPos)
    {
        SuppressDoingPhysics scopeGuard;

        // remove active
        mEnvironment.mMechanicsManager->removeActor (mPlayer->getPlayer());

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
                    mExteriors[std::make_pair (x, y)].loadExt (x, y, mStore, mEsm);
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
        adjustSky();

        mCellChanged = true;
    }

    World::World (OEngine::Render::OgreRenderer& renderer, const boost::filesystem::path& dataDir,
        const std::string& master, const boost::filesystem::path& resDir,
        bool newGame, Environment& environment)
    : mSkyManager (0), mScene (renderer), mPlayer (0), mCurrentCell (0), mGlobalVariables (0),
      mSky (false), mCellChanged (false), mEnvironment (environment)
    {
        boost::filesystem::path masterPath (dataDir);
        masterPath /= master;

        std::cout << "Loading ESM " << masterPath.string() << "\n";

        // This parses the ESM file and loads a sample cell
        mEsm.open (masterPath.file_string());
        mStore.load (mEsm);

        mPlayer = new MWWorld::Player (mScene.getPlayer(), mStore.npcs.find ("player"), *this);
        mScene.addActor (mPlayer->getPlayer().getRefData().getHandle(), "", Ogre::Vector3 (0, 0, 0));

        // global variables
        mGlobalVariables = new Globals (mStore);

        if (newGame)
        {
            // set new game mark
            mGlobalVariables->setInt ("chargenstate", 1);
        }

        mSkyManager =
            MWRender::SkyManager::create(renderer.getWindow(), mScene.getCamera(), resDir);
    }

    World::~World()
    {
        for (CellRenderCollection::iterator iter (mActiveCells.begin());
            iter!=mActiveCells.end(); ++iter)
            delete iter->second;

        for (CellRenderCollection::iterator iter (mBufferedCells.begin());
            iter!=mBufferedCells.end(); ++iter)
            delete iter->second;

        delete mPlayer;
        delete mSkyManager;
        delete mGlobalVariables;
    }

    MWWorld::Player& World::getPlayer()
    {
        return *mPlayer;
    }

    ESMS::ESMStore& World::getStore()
    {
        return mStore;
    }

    const World::ScriptList& World::getLocalScripts() const
    {
        return mLocalScripts;
    }

    bool World::hasCellChanged() const
    {
        return mCellChanged;
    }

    Globals::Data& World::getGlobalVariable (const std::string& name)
    {
        return (*mGlobalVariables)[name];
    }

    char World::getGlobalVariableType (const std::string& name) const
    {
        return mGlobalVariables->getType (name);
    }

    Ptr World::getPtr (const std::string& name, bool activeOnly)
    {
        // the player is always in an active cell.
        if (name=="player")
        {
            return mPlayer->getPlayer();
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

    Ptr World::getPtrViaHandle (const std::string& handle)
    {
        if (mPlayer->getPlayer().getRefData().getHandle()==handle)
            return mPlayer->getPlayer();

        for (CellRenderCollection::iterator iter (mActiveCells.begin());
            iter!=mActiveCells.end(); ++iter)
        {
            Ptr ptr = getPtrViaHandle (handle, *iter->first);

            if (!ptr.isEmpty())
                return ptr;
        }

        throw std::runtime_error ("unknown Ogre handle: " + handle);
    }

    void World::enable (Ptr reference)
    {
        if (!reference.getRefData().isEnabled())
        {
            reference.getRefData().enable();

            if (MWRender::CellRender *render = searchRender (reference.getCell()))
            {
                render->enable (reference.getRefData().getHandle());

                if (mActiveCells.find (reference.getCell())!=mActiveCells.end())
                    Class::get (reference).enable (reference, mEnvironment);
            }
        }
    }

    void World::disable (Ptr reference)
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

    void World::advanceTime (double hours)
    {
        hours += mGlobalVariables->getFloat ("gamehour");

        setHour (hours);

        int days = hours / 24;

        if (days>0)
            mGlobalVariables->setInt ("dayspassed", days + mGlobalVariables->getInt ("dayspassed"));
    }

    void World::setHour (double hour)
    {
        if (hour<0)
            hour = 0;

        int days = hour / 24;

        hour = std::fmod (hour, 24);

        mGlobalVariables->setFloat ("gamehour", hour);

        mSkyManager->setHour (hour);

        if (days>0)
            setDay (days + mGlobalVariables->getInt ("day"));
    }

    void World::setDay (int day)
    {
        if (day<0)
            day = 0;

        int month = mGlobalVariables->getInt ("month");

        while (true)
        {
            int days = getDaysPerMonth (month);
            if (day<days)
                break;

            if (month<11)
            {
                ++month;
            }
            else
            {
                month = 0;
                mGlobalVariables->setInt ("year", mGlobalVariables->getInt ("year")+1);
            }

            day -= days;
        }

        mGlobalVariables->setInt ("day", day);
        mGlobalVariables->setInt ("month", month);

        mSkyManager->setDate (day, month);
    }

    void World::setMonth (int month)
    {
        if (month<0)
            month = 0;

        int years = month / 12;
        month = month % 12;

        int days = getDaysPerMonth (month);

        if (mGlobalVariables->getInt ("day")>=days)
            mGlobalVariables->setInt ("day", days-1);

        mGlobalVariables->setInt ("month", month);

        if (years>0)
            mGlobalVariables->setInt ("year", years+mGlobalVariables->getInt ("year"));

        mSkyManager->setDate (mGlobalVariables->getInt ("day"), month);
    }

    void World::toggleSky()
    {
        if (mSky)
        {
            mSky = false;
            mSkyManager->disable();
        }
        else
        {
            mSky = true;
            // TODO check for extorior or interior with sky.
            mSkyManager->setHour (mGlobalVariables->getFloat ("gamehour"));
            mSkyManager->setDate (mGlobalVariables->getInt ("day"),
                mGlobalVariables->getInt ("month"));
            mSkyManager->enable();
        }
    }

    int World::getMasserPhase() const
    {
        return mSkyManager->getMasserPhase();
    }

    int World::getSecundaPhase() const
    {
        return mSkyManager->getSecundaPhase();
    }

    void World::setMoonColour (bool red)
    {
        mSkyManager->setMoonColour (red);
    }

    float World::getTimeScaleFactor() const
    {
        return mGlobalVariables->getInt ("timescale");
    }

    void World::changeToInteriorCell (const std::string& cellName, const ESM::Position& position)
    {
        SuppressDoingPhysics scopeGuard;

        // remove active
        CellRenderCollection::iterator active = mActiveCells.begin();

        while (active!=mActiveCells.end())
        {
            unloadCell (active++);
        }

        // Load cell.
        mInteriors[cellName].loadInt (cellName, mStore, mEsm);
        Ptr::CellStore *cell = &mInteriors[cellName];

        loadCell (cell, new MWRender::InteriorCellRender (*cell, mEnvironment, mScene));

        // adjust player
        mCurrentCell = cell;
        playerCellChange (cell, position);

        // Sky system
        adjustSky();

        mCellChanged = true;
        //currentRegion->name = "";
    }

    void World::changeToExteriorCell (const ESM::Position& position)
    {
        int x = 0;
        int y = 0;

        positionToIndex (position.pos[0], position.pos[1], x, y);

        changeCell (x, y, position, true);
    }

    const ESM::Cell *World::getExterior (const std::string& cellName) const
    {
        // first try named cells
        if (const ESM::Cell *cell = mStore.cells.searchExtByName (cellName))
            return cell;

        // didn't work -> now check for regions
        std::string cellName2 = ESMS::RecListT<ESM::Region>::toLower (cellName);

        for (ESMS::RecListT<ESM::Region>::MapType::const_iterator iter (mStore.regions.list.begin());
            iter!=mStore.regions.list.end(); ++iter)
        {
            if (ESMS::RecListT<ESM::Region>::toLower (iter->second.name)==cellName2)
            {
                if (const ESM::Cell *cell = mStore.cells.searchExtByRegion (iter->first))
                    return cell;

                break;
            }
        }

        return 0;
    }

    void World::markCellAsUnchanged()
    {
        mCellChanged = false;
    }

    std::string World::getFacedHandle()
    {
        std::pair<std::string, float> result = mScene.getFacedHandle (*this);

        if (result.first.empty() ||
            result.second>getStore().gameSettings.find ("iMaxActivateDist")->i)
            return "";

        return result.first;
    }

    void World::deleteObject (Ptr ptr)
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

    void World::moveObject (Ptr ptr, float x, float y, float z)
    {
        ptr.getCellRef().pos.pos[0] = x;
        ptr.getCellRef().pos.pos[1] = y;
        ptr.getCellRef().pos.pos[2] = z;

        if (ptr==mPlayer->getPlayer())
        {
            if (mCurrentCell)
            {
                if (!(mCurrentCell->cell->data.flags & ESM::Cell::Interior))
                {
                    // exterior -> adjust loaded cells
                    int cellX = 0;
                    int cellY = 0;

                    positionToIndex (x, y, cellX, cellY);

                    if (mCurrentCell->cell->data.gridX!=cellX || mCurrentCell->cell->data.gridY!=cellY)
                    {
                        changeCell (cellX, cellY, mPlayer->getPlayer().getCellRef().pos, false);
                    }

                }
            }
        }

        mScene.moveObject (ptr.getRefData().getHandle(), Ogre::Vector3 (x, y, z),
            !DoingPhysics::isDoingPhysics());

        // TODO cell change for non-player ref
    }

    void World::indexToPosition (int cellX, int cellY, float &x, float &y, bool centre) const
    {
        const int cellSize = 8192;

        x = cellSize * cellX;
        y = cellSize * cellY;

        if (centre)
        {
            x += cellSize/2;
            y += cellSize/2;
        }
    }

    void World::positionToIndex (float x, float y, int &cellX, int &cellY) const
    {
        const int cellSize = 8192;

        cellX = static_cast<int> (x/cellSize);

        if (x<0)
            --cellX;

        cellY = static_cast<int> (y/cellSize);

        if (y<0)
            --cellY;
    }

    void World::doPhysics (const std::vector<std::pair<std::string, Ogre::Vector3> >& actors,
        float duration)
    {
        mScene.doPhysics (duration, *this, actors);
    }
}
