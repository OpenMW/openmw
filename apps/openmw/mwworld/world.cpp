#include "world.hpp"

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

    MWRender::CellRender *World::searchRender (Ptr::CellStore *store)
    {
        CellRenderCollection::iterator iter = mWorldScene->getActiveCells().find (store);

        if (iter!=mWorldScene->getActiveCells().end())
        {
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


    void World::adjustSky()
    {
        if (mSky)
        {
            toggleSky();
            // TODO set weather
            toggleSky();
        }
    }

    World::World (OEngine::Render::OgreRenderer& renderer, OEngine::Physic::PhysicEngine* physEng,
        const Files::Collections& fileCollections,
        const std::string& master, const boost::filesystem::path& resDir,
        bool newGame, Environment& environment, const std::string& encoding)
    : mSkyManager (0), mScene (renderer,physEng), mPlayer (0), mGlobalVariables (0),
      mSky (false), mEnvironment (environment), mNextDynamicRecord (0)
    {
        mPhysEngine = physEng;

        boost::filesystem::path masterPath (fileCollections.getCollection (".esm").getPath (master));

        std::cout << "Loading ESM " << masterPath.string() << "\n";

        // This parses the ESM file and loads a sample cell
        mEsm.setEncoding(encoding);
        mEsm.open (masterPath.string());
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

        mPhysEngine = physEng;
        
        mWorldScene = new Scene(environment, this, mScene);
    }

    World::~World()
    {
        delete mPlayer;
        delete mSkyManager;
        delete mGlobalVariables;
        delete mWorldScene;
    }

    MWWorld::Player& World::getPlayer()
    {
        return *mPlayer;
    }

    const ESMS::ESMStore& World::getStore() const
    {
        return mStore;
    }
    
    ESM::ESMReader& World::getEsmReader()
    {
        return mEsm;
    }

    const World::ScriptList& World::getLocalScripts() const
    {
        return mLocalScripts;
    }

    bool World::hasCellChanged() const
    {
        return mWorldScene->hasCellChanged();
    }

    Globals::Data& World::getGlobalVariable (const std::string& name)
    {
        return (*mGlobalVariables)[name];
    }

    Globals::Data World::getGlobalVariable (const std::string& name) const
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
        for (CellRenderCollection::iterator iter (mWorldScene->getActiveCells().begin());
            iter!=mWorldScene->getActiveCells().end(); ++iter)
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

        for (CellRenderCollection::iterator iter (mWorldScene->getActiveCells().begin());
            iter!=mWorldScene->getActiveCells().end(); ++iter)
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

                if (mWorldScene->getActiveCells().find (reference.getCell())!=mWorldScene->getActiveCells().end())
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

                if (mWorldScene->getActiveCells().find (reference.getCell())!=mWorldScene->getActiveCells().end())
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

    bool World::toggleSky()
    {
        if (mSky)
        {
            mSky = false;
            mSkyManager->disable();
            return false;
        }
        else
        {
            mSky = true;
            // TODO check for extorior or interior with sky.
            mSkyManager->setHour (mGlobalVariables->getFloat ("gamehour"));
            mSkyManager->setDate (mGlobalVariables->getInt ("day"),
                mGlobalVariables->getInt ("month"));
            mSkyManager->enable();
            return true;
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
        return mWorldScene->changeToInteriorCell(cellName, position);
    }

    void World::changeToExteriorCell (const ESM::Position& position)
    {
        return mWorldScene->changeToExteriorCell(position);
    }

    const ESM::Cell *World::getExterior (const std::string& cellName) const
    {
        return mWorldScene->getExterior(cellName);
    }

    void World::markCellAsUnchanged()
    {
        return mWorldScene->markCellAsUnchanged();
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
                if (mWorldScene->getActiveCells().find (ptr.getCell())!=mWorldScene->getActiveCells().end())
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
            Ptr::CellStore *currentCell = mWorldScene->getCurrentCell();
            if (currentCell)
            {
                if (!(currentCell->cell->data.flags & ESM::Cell::Interior))
                {
                    // exterior -> adjust loaded cells
                    int cellX = 0;
                    int cellY = 0;

                    positionToIndex (x, y, cellX, cellY);

                    if (currentCell->cell->data.gridX!=cellX || currentCell->cell->data.gridY!=cellY)
                    {
                        mWorldScene->changeCell (cellX, cellY, mPlayer->getPlayer().getCellRef().pos, false);
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

    bool World::toggleCollisionMode()
    {
        return mScene.toggleCollisionMode();
    }

    bool World::toggleRenderMode (RenderMode mode)
    {
        return mScene.toggleRenderMode (mode);
    }

    std::pair<std::string, const ESM::Potion *> World::createRecord (const ESM::Potion& record)
    {
        /// \todo Rewrite the ESMStore so that a dynamic 2nd ESMStore can be attached to it.
        /// This function should then insert the record into the 2nd store (the code for this
        /// should also be moved to the ESMStore class). It might be a good idea to review
        /// the STL-container usage of the ESMStore before the rewrite.

        std::ostringstream stream;
        stream << "$dynamic" << mNextDynamicRecord++;

        const ESM::Potion *created =
            &mStore.potions.list.insert (std::make_pair (stream.str(), record)).first->second;

        mStore.all.insert (std::make_pair (stream.str(), ESM::REC_ALCH));

        return std::make_pair (stream.str(), created);
    }

    std::pair<std::string, const ESM::Class *> World::createRecord (const ESM::Class& record)
    {
        /// \todo See function above.
        std::ostringstream stream;
        stream << "$dynamic" << mNextDynamicRecord++;

        const ESM::Class *created =
            &mStore.classes.list.insert (std::make_pair (stream.str(), record)).first->second;

        mStore.all.insert (std::make_pair (stream.str(), ESM::REC_CLAS));

        return std::make_pair (stream.str(), created);
    }
}

