#include "world.hpp"

#include <cmath>
#include <iostream>

#include <components/bsa/bsa_archive.hpp>
#include <components/files/collections.hpp>

#include "../mwrender/sky.hpp"
#include "../mwrender/player.hpp"

#include "../mwmechanics/mechanicsmanager.hpp"

#include "../mwsound/soundmanager.hpp"


#include "ptr.hpp"
#include "environment.hpp"
#include "class.hpp"
#include "player.hpp"
#include "weather.hpp"

#include "refdata.hpp"
#include "globals.hpp"
#include "cellfunctors.hpp"

namespace
{
    template<typename T>
    void listCellScripts (const ESMS::ESMStore& store,
        ESMS::CellRefList<T, MWWorld::RefData>& cellRefList, MWWorld::LocalScripts& localScripts,
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

                    localScripts.add (iter->base->script, MWWorld::Ptr (&*iter, cell));
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
            if(iter->mData.getBaseNode()){
            if (iter->mData.getHandle()==handle)
            {
                return &*iter;
            }
            }
        }
        return 0;
    }
}

namespace MWWorld
{
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

    void World::adjustSky()
    {
        if (mSky)
        {
            toggleSky();
            // TODO set weather
            toggleSky();
        }
    }

    World::World (OEngine::Render::OgreRenderer& renderer,
        const Files::Collections& fileCollections,
        const std::string& master, const boost::filesystem::path& resDir,
        bool newGame, Environment& environment, const std::string& encoding)
    : mPlayer (0), mLocalScripts (mStore), mGlobalVariables (0),
      mSky (false), mEnvironment (environment), mNextDynamicRecord (0), mCells (mStore, mEsm, *this)
    {
        mPhysics = new PhysicsSystem(renderer);
        mPhysEngine = mPhysics->getEngine();
        
        mRendering = new MWRender::RenderingManager(renderer, resDir, mPhysEngine, environment);
        
        mWeatherManager = new MWWorld::WeatherManager(mRendering);

        boost::filesystem::path masterPath (fileCollections.getCollection (".esm").getPath (master));

        std::cout << "Loading ESM " << masterPath.string() << "\n";

        // This parses the ESM file and loads a sample cell
        mEsm.setEncoding(encoding);
        mEsm.open (masterPath.string());
        mStore.load (mEsm);

        MWRender::Player* play = &(mRendering->getPlayer());
        mPlayer = new MWWorld::Player (play, mStore.npcs.find ("player"), *this);
        mPhysics->addActor (mPlayer->getPlayer().getRefData().getHandle(), "", Ogre::Vector3 (0, 0, 0));

        // global variables
        mGlobalVariables = new Globals (mStore);

        if (newGame)
        {
            // set new game mark
            mGlobalVariables->setInt ("chargenstate", 1);
        }

        mWorldScene = new Scene(environment, this, *mRendering, mPhysics);

    }


    World::~World()
    {
        delete mWeatherManager;
        delete mWorldScene;
        delete mGlobalVariables;
        delete mRendering;
        delete mPhysics;

        delete mPlayer;
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

    Ptr::CellStore *World::getExterior (int x, int y)
    {
        return mCells.getExterior (x, y);
    }

    Ptr::CellStore *World::getInterior (const std::string& name)
    {
        return mCells.getInterior (name);
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

    LocalScripts& World::getLocalScripts()
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
        for (Scene::CellStoreCollection::const_iterator iter (mWorldScene->getActiveCells().begin());
            iter!=mWorldScene->getActiveCells().end(); ++iter)
        {
            Ptr::CellStore* cellstore = *iter;
            Ptr ptr = mCells.getPtr (name, *cellstore);

            if (!ptr.isEmpty())
                return ptr;
        }

        if (!activeOnly)
        {
            Ptr ptr = mCells.getPtr (name);

            if (!ptr.isEmpty())
                return ptr;
        }

        throw std::runtime_error ("unknown ID: " + name);
    }

    Ptr World::getPtrViaHandle (const std::string& handle)
    {
        if (mPlayer->getPlayer().getRefData().getHandle()==handle)
            return mPlayer->getPlayer();
        for (Scene::CellStoreCollection::const_iterator iter (mWorldScene->getActiveCells().begin());
            iter!=mWorldScene->getActiveCells().end(); ++iter)
        {
            Ptr::CellStore* cellstore = *iter;
            Ptr ptr = getPtrViaHandle (handle, *cellstore);

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


                //render->enable (reference.getRefData().getHandle());
            if(mWorldScene->getActiveCells().find (reference.getCell()) != mWorldScene->getActiveCells().end())
                 Class::get (reference).enable (reference, mEnvironment);


        }
    }

    void World::disable (Ptr reference)
    {
        if (reference.getRefData().isEnabled())
        {
            reference.getRefData().disable();


                //render->disable (reference.getRefData().getHandle());
            if(mWorldScene->getActiveCells().find (reference.getCell())!=mWorldScene->getActiveCells().end()){
                  Class::get (reference).disable (reference, mEnvironment);
                  mEnvironment.mSoundManager->stopSound3D (reference);
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

        mRendering->skySetHour (hour);
        
        mWeatherManager->setHour (hour);

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

        mRendering->skySetDate (day, month);
        
        mWeatherManager->setDate (day, month);
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

        mRendering->skySetDate (mGlobalVariables->getInt ("day"), month);
    }

    bool World::toggleSky()
    {
        if (mSky)
        {
            mSky = false;
            mRendering->skyDisable();
            return false;
        }
        else
        {
            mSky = true;
            // TODO check for extorior or interior with sky.
            mRendering->skySetHour (mGlobalVariables->getFloat ("gamehour"));
            mRendering->skySetDate (mGlobalVariables->getInt ("day"),
                mGlobalVariables->getInt ("month"));
            mRendering->skyEnable();
            return true;
        }
    }

    int World::getMasserPhase() const
    {
        return mRendering->skyGetMasserPhase();
    }

    int World::getSecundaPhase() const
    {
        return mRendering->skyGetSecundaPhase();
    }

    void World::setMoonColour (bool red)
    {
        mRendering->skySetMoonColour (red);
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

    void World::markCellAsUnchanged()
    {
        return mWorldScene->markCellAsUnchanged();
    }

    std::string World::getFacedHandle()
    {
        std::pair<std::string, float> result = mPhysics->getFacedHandle (*this);

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


                if (mWorldScene->getActiveCells().find (ptr.getCell())!=mWorldScene->getActiveCells().end()){
//                           Class::get (ptr).disable (ptr, mEnvironment); /// \todo this line needs to be removed
                            mEnvironment.mSoundManager->stopSound3D (ptr);

                            mPhysics->removeObject (ptr.getRefData().getHandle());
                            mRendering->removeObject(ptr);

                            mLocalScripts.remove (ptr);
                }
        }
    }

    void World::moveObjectImp (Ptr ptr, float x, float y, float z)
    {
        ptr.getRefData().getPosition().pos[0] = x;
        ptr.getRefData().getPosition().pos[1] = y;
        ptr.getRefData().getPosition().pos[2] = z;

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
                        mWorldScene->changeCell (cellX, cellY, mPlayer->getPlayer().getRefData().getPosition(), false);
                    }

                }
            }
        }

        /// \todo cell change for non-player ref

        mRendering->moveObject (ptr, Ogre::Vector3 (x, y, z));
    }

    void World::moveObject (Ptr ptr, float x, float y, float z)
    {
        moveObjectImp(ptr, x, y, z);

        mPhysics->moveObject (ptr.getRefData().getHandle(), Ogre::Vector3 (x, y, z));
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
        std::vector< std::pair<std::string, Ogre::Vector3> > vectors = mPhysics->doPhysics (duration, actors);

        std::vector< std::pair<std::string, Ogre::Vector3> >::iterator player = vectors.end();

        for (std::vector< std::pair<std::string, Ogre::Vector3> >::iterator it = vectors.begin();
            it!= vectors.end(); ++it)
        {
            if (it->first=="player")
            {
                player = it;
            }
            else
            {
                MWWorld::Ptr ptr = getPtrViaHandle (it->first);
                moveObjectImp (ptr, it->second.x, it->second.y, it->second.z);
            }
        }

        // Make sure player is moved last (otherwise the cell might change in the middle of an update
        // loop)
        if (player!=vectors.end())
            moveObjectImp (getPtrViaHandle (player->first),
                player->second.x, player->second.y, player->second.z);
    }

    bool World::toggleCollisionMode()
    {
        return mPhysics->toggleCollisionMode();;
    }

    bool World::toggleRenderMode (RenderMode mode)
    {
        return mRendering->toggleRenderMode (mode);
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

    const ESM::Cell *World::createRecord (const ESM::Cell& record)
    {
        if (record.data.flags & ESM::Cell::Interior)
        {
            if (mStore.cells.searchInt (record.name))
                throw std::runtime_error ("failed creating interior cell");

            ESM::Cell *cell = new ESM::Cell (record);
            mStore.cells.intCells.insert (std::make_pair (record.name, cell));
            return cell;
        }
        else
        {
            if (mStore.cells.searchExt (record.data.gridX, record.data.gridY))
                throw std::runtime_error ("failed creating exterior cell");

            ESM::Cell *cell = new ESM::Cell (record);
            mStore.cells.extCells.insert (
                std::make_pair (std::make_pair (record.data.gridX, record.data.gridY), cell));
            return cell;
        }
    }

    void World::playAnimationGroup (const MWWorld::Ptr& ptr, const std::string& groupName, int mode,
        int number)
    {
        mRendering->playAnimationGroup (ptr, groupName, mode, number);
    }

    void World::skipAnimation (const MWWorld::Ptr& ptr)
    {
        mRendering->skipAnimation (ptr);
    }

    void World::update (float duration)
    {
        mWorldScene->update (duration);
        
        mWeatherManager->update (duration);
    }
    
    OEngine::Render::Fader* World::getFader()
    {
        return mRendering->getFader();
    }
}
