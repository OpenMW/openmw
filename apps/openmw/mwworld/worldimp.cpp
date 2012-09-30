#include "worldimp.hpp"

#include <components/bsa/bsa_archive.hpp>
#include <components/files/collections.hpp>

#include "../mwbase/environment.hpp"
#include "../mwbase/soundmanager.hpp"
#include "../mwbase/mechanicsmanager.hpp"
#include "../mwbase/windowmanager.hpp"

#include "../mwrender/sky.hpp"
#include "../mwrender/player.hpp"

#include "player.hpp"
#include "manualref.hpp"
#include "cellfunctors.hpp"

using namespace Ogre;

namespace
{
    template<typename T>
    void listCellScripts (const ESMS::ESMStore& store,
        MWWorld::CellRefList<T>& cellRefList, MWWorld::LocalScripts& localScripts,
        MWWorld::Ptr::CellStore *cell)
    {
        for (typename MWWorld::CellRefList<T>::List::iterator iter (
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
    MWWorld::LiveCellRef<T> *searchViaHandle (const std::string& handle,
        MWWorld::CellRefList<T>& refList)
    {
        typedef typename MWWorld::CellRefList<T>::List::iterator iterator;

        for (iterator iter (refList.list.begin()); iter!=refList.list.end(); ++iter)
        {
            if(iter->mData.getCount() > 0 && iter->mData.getBaseNode()){
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
        if (MWWorld::LiveCellRef<ESM::Activator> *ref =
            searchViaHandle (handle, cell.activators))
            return Ptr (ref, &cell);
        if (MWWorld::LiveCellRef<ESM::Potion> *ref = searchViaHandle (handle, cell.potions))
            return Ptr (ref, &cell);
        if (MWWorld::LiveCellRef<ESM::Apparatus> *ref = searchViaHandle (handle, cell.appas))
            return Ptr (ref, &cell);
        if (MWWorld::LiveCellRef<ESM::Armor> *ref = searchViaHandle (handle, cell.armors))
            return Ptr (ref, &cell);
        if (MWWorld::LiveCellRef<ESM::Book> *ref = searchViaHandle (handle, cell.books))
            return Ptr (ref, &cell);
        if (MWWorld::LiveCellRef<ESM::Clothing> *ref = searchViaHandle (handle, cell.clothes))
            return Ptr (ref, &cell);
        if (MWWorld::LiveCellRef<ESM::Container> *ref =
            searchViaHandle (handle, cell.containers))
            return Ptr (ref, &cell);
        if (MWWorld::LiveCellRef<ESM::Creature> *ref =
            searchViaHandle (handle, cell.creatures))
            return Ptr (ref, &cell);
        if (MWWorld::LiveCellRef<ESM::Door> *ref = searchViaHandle (handle, cell.doors))
            return Ptr (ref, &cell);
        if (MWWorld::LiveCellRef<ESM::Ingredient> *ref =
            searchViaHandle (handle, cell.ingreds))
            return Ptr (ref, &cell);
        if (MWWorld::LiveCellRef<ESM::Light> *ref = searchViaHandle (handle, cell.lights))
            return Ptr (ref, &cell);
        if (MWWorld::LiveCellRef<ESM::Tool> *ref = searchViaHandle (handle, cell.lockpicks))
            return Ptr (ref, &cell);
        if (MWWorld::LiveCellRef<ESM::Miscellaneous> *ref = searchViaHandle (handle, cell.miscItems))
            return Ptr (ref, &cell);
        if (MWWorld::LiveCellRef<ESM::NPC> *ref = searchViaHandle (handle, cell.npcs))
            return Ptr (ref, &cell);
        if (MWWorld::LiveCellRef<ESM::Probe> *ref = searchViaHandle (handle, cell.probes))
            return Ptr (ref, &cell);
        if (MWWorld::LiveCellRef<ESM::Repair> *ref = searchViaHandle (handle, cell.repairs))
            return Ptr (ref, &cell);
        if (MWWorld::LiveCellRef<ESM::Static> *ref = searchViaHandle (handle, cell.statics))
            return Ptr (ref, &cell);
        if (MWWorld::LiveCellRef<ESM::Weapon> *ref = searchViaHandle (handle, cell.weapons))
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
        if (mSky && (isCellExterior() || isCellQuasiExterior()))
        {
            mRendering->skySetHour (mGlobalVariables->getFloat ("gamehour"));
            mRendering->skySetDate (mGlobalVariables->getInt ("day"),
                mGlobalVariables->getInt ("month"));

            mRendering->skyEnable();
        }
        else
            mRendering->skyDisable();
    }

    void World::setFallbackValues (const std::map<std::string,std::string>& fallbackMap)
    {
        mFallback = fallbackMap;
    }

    std::string World::getFallback (const std::string& key) const
    {
        return getFallback(key, "");
    }

    std::string World::getFallback (const std::string& key, const std::string& def) const
    {
        std::map<std::string,std::string>::const_iterator it;
        if((it = mFallback.find(key)) == mFallback.end())
        {
            return def;
        }
        return it->second;
    }

    World::World (OEngine::Render::OgreRenderer& renderer,
        const Files::Collections& fileCollections,
        const std::string& master, const boost::filesystem::path& resDir, const boost::filesystem::path& cacheDir, bool newGame,
        const std::string& encoding, std::map<std::string,std::string> fallbackMap)
    : mPlayer (0), mLocalScripts (mStore), mGlobalVariables (0),
      mSky (true), mNextDynamicRecord (0), mCells (mStore, mEsm),
      mNumFacing(0)
    {
        mPhysics = new PhysicsSystem(renderer);
        mPhysEngine = mPhysics->getEngine();

        mRendering = new MWRender::RenderingManager(renderer, resDir, cacheDir, mPhysEngine);

        mWeatherManager = new MWWorld::WeatherManager(mRendering);

        boost::filesystem::path masterPath (fileCollections.getCollection (".esm").getPath (master));

        std::cout << "Loading ESM " << masterPath.string() << "\n";

        // This parses the ESM file and loads a sample cell
        mEsm.setEncoding(encoding);
        mEsm.open (masterPath.string());
        mStore.load (mEsm);

        mPlayer = new MWWorld::Player (mStore.npcs.find ("player"), *this);
        mRendering->attachCameraTo(mPlayer->getPlayer());

        std::string playerCollisionFile = "meshes\\base_anim.nif";    //This is used to make a collision shape for our player
                                                                      //We will need to support the 1st person file too in the future
        mPhysics->addActor (mPlayer->getPlayer().getRefData().getHandle(), playerCollisionFile, Ogre::Vector3 (0, 0, 0), 1, Ogre::Quaternion::ZERO);

        // global variables
        mGlobalVariables = new Globals (mStore);

        if (newGame)
        {
            // set new game mark
            mGlobalVariables->setInt ("chargenstate", 1);
        }

        mWorldScene = new Scene(*mRendering, mPhysics);

        setFallbackValues(fallbackMap);

        lastTick = mTimer.getMilliseconds();
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
            if (ESMS::RecListT<ESM::Region>::toLower (iter->second.mName)==cellName2)
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

    void World::enable (const Ptr& reference)
    {
        if (!reference.getRefData().isEnabled())
        {
            reference.getRefData().enable();

            if(mWorldScene->getActiveCells().find (reference.getCell()) != mWorldScene->getActiveCells().end() && reference.getRefData().getCount())
                mWorldScene->addObjectToScene (reference);
        }
    }

    void World::disable (const Ptr& reference)
    {
        if (reference.getRefData().isEnabled())
        {
            reference.getRefData().disable();

            if(mWorldScene->getActiveCells().find (reference.getCell())!=mWorldScene->getActiveCells().end() && reference.getRefData().getCount())
                mWorldScene->removeObjectFromScene (reference);
        }
    }

    void World::advanceTime (double hours)
    {
        mWeatherManager->advanceTime (hours);

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

    int World::getDay()
    {
        return mGlobalVariables->getInt("day");
    }

    int World::getMonth()
    {
        return mGlobalVariables->getInt("month");
    }

    TimeStamp World::getTimeStamp() const
    {
        return TimeStamp (mGlobalVariables->getFloat ("gamehour"),
            mGlobalVariables->getInt ("dayspassed"));
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
        return mGlobalVariables->getFloat ("timescale");
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
        if (!mRendering->occlusionQuerySupported())
        {
            std::pair<std::string, float> result = mPhysics->getFacedHandle (*this);

            if (result.first.empty() ||
                    result.second>getStore().gameSettings.find ("iMaxActivateDist")->getInt())
                return "";

            return result.first;
        }
        else
        {
            // updated every few frames in update()
            return mFacedHandle;
        }
    }

    void World::deleteObject (const Ptr& ptr)
    {
        if (ptr.getRefData().getCount()>0)
        {
            ptr.getRefData().setCount (0);

            if (mWorldScene->getActiveCells().find (ptr.getCell())!=mWorldScene->getActiveCells().end() &&
                ptr.getRefData().isEnabled())
            {
                mWorldScene->removeObjectFromScene (ptr);
                mLocalScripts.remove (ptr);
            }
        }
    }

    std::string toLower (const std::string& name)
    {
        std::string lowerCase;

        std::transform (name.begin(), name.end(), std::back_inserter (lowerCase),
            (int(*)(int)) std::tolower);

        return lowerCase;
    }

    void World::moveObject(const Ptr &ptr, CellStore &newCell, float x, float y, float z)
    {
        ESM::Position &pos = ptr.getRefData().getPosition();
        pos.pos[0] = x, pos.pos[1] = y, pos.pos[2] = z;
        Ogre::Vector3 vec(x, y, z);

        CellStore *currCell = ptr.getCell();
        bool isPlayer = ptr == mPlayer->getPlayer();
        bool haveToMove = mWorldScene->isCellActive(*currCell) || isPlayer;
        if (*currCell != newCell) {
            if (isPlayer) {
                if (!newCell.isExterior()) {
                    changeToInteriorCell(toLower(newCell.cell->mName), pos);
                } else {
                    int cellX = newCell.cell->mData.mX;
                    int cellY = newCell.cell->mData.mY;
                    mWorldScene->changeCell(cellX, cellY, pos, false);
                }
            } else {
                if (!mWorldScene->isCellActive(*currCell)) {
                    copyObjectToCell(ptr, newCell, pos);
                } else if (!mWorldScene->isCellActive(newCell)) {
                    MWWorld::Class::get(ptr).copyToCell(ptr, newCell);
                    mWorldScene->removeObjectFromScene(ptr);
                    mLocalScripts.remove(ptr);
                    haveToMove = false;
                } else {
                    MWWorld::Ptr copy =
                        MWWorld::Class::get(ptr).copyToCell(ptr, newCell);

                    mRendering->moveObjectToCell(copy, vec, currCell);

                    if (MWWorld::Class::get(ptr).isActor()) {
                        MWBase::MechanicsManager *mechMgr =
                            MWBase::Environment::get().getMechanicsManager();

                        mechMgr->removeActor(ptr);
                        mechMgr->addActor(copy);
                    } else {
                        std::string script =
                            MWWorld::Class::get(ptr).getScript(ptr);
                        if (!script.empty()) {
                            mLocalScripts.remove(ptr);
                            mLocalScripts.add(script, copy);
                        }
                    }
                }
                ptr.getRefData().setCount(0);
            }
        }
        if (haveToMove) {
            mRendering->moveObject(ptr, vec);
            mPhysics->moveObject (ptr.getRefData().getHandle(), ptr.getRefData().getBaseNode());
        }
    }

    bool World::moveObjectImp(const Ptr& ptr, float x, float y, float z)
    {
        CellStore *cell = ptr.getCell();
        if (cell->isExterior()) {
            int cellX, cellY;
            positionToIndex(x, y, cellX, cellY);

            cell = getExterior(cellX, cellY);
        }
        moveObject(ptr, *cell, x, y, z);

        return cell != ptr.getCell();
    }

    void World::moveObject (const Ptr& ptr, float x, float y, float z)
    {
        moveObjectImp(ptr, x, y, z);

        
    }

    void World::scaleObject (const Ptr& ptr, float scale)
    {
        MWWorld::Class::get(ptr).adjustScale(ptr,scale);

        ptr.getCellRef().mScale = scale;
        //scale = scale/ptr.getRefData().getBaseNode()->getScale().x;
        ptr.getRefData().getBaseNode()->setScale(scale,scale,scale);
        mPhysics->scaleObject( ptr.getRefData().getHandle(), ptr.getRefData().getBaseNode());
    }

    void World::rotateObject (const Ptr& ptr,float x,float y,float z, bool adjust)
    {
        Ogre::Vector3 rot;
        rot.x = Ogre::Degree(x).valueRadians();
        rot.y = Ogre::Degree(y).valueRadians();
        rot.z = Ogre::Degree(z).valueRadians();
        
        if (mRendering->rotateObject(ptr, rot, adjust)) {
            float *objRot = ptr.getRefData().getPosition().rot;
            objRot[0] = rot.x, objRot[1] = rot.y, objRot[2] = rot.z;


            if (ptr.getRefData().getBaseNode() != 0) {
                mPhysics->rotateObject(
                    ptr.getRefData().getHandle(),
                    ptr.getRefData().getBaseNode()
                );
            }
        }

    }

    void World::safePlaceObject(const MWWorld::Ptr& ptr,MWWorld::CellStore &Cell,ESM::Position pos)
    {
        copyObjectToCell(ptr,Cell,pos);
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
        mPhysics->doPhysics(duration, actors);

        const int tick = 16; // 16 ms ^= 60 Hz

        // Game clock part of the loop, contains everything that has to be executed in a fixed timestep
        long long dt = mTimer.getMilliseconds() - lastTick;
        if (dt >= 100)
        {
            //  throw away wall clock time if necessary to keep the framerate above the minimum of 10 fps
            lastTick += (dt - 100);
            dt = 100;
        }
        while (dt >= tick)
        {
            dt -= tick;
            lastTick += tick;

            std::vector< std::pair<std::string, Ogre::Vector3> > vectors = mPhysics->doPhysicsFixed (actors);

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
            {
                if (moveObjectImp (getPtrViaHandle (player->first),
                    player->second.x, player->second.y, player->second.z) == true)
                    return; // abort the current loop if the cell has changed
            }
        }
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

        ESM::Potion record2 (record);
        record2.mId = stream.str();

        const ESM::Potion *created =
            &mStore.potions.list.insert (std::make_pair (stream.str(), record2)).first->second;

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
        if (record.mData.mFlags & ESM::Cell::Interior)
        {
            if (mStore.cells.searchInt (record.mName))
                throw std::runtime_error ("failed creating interior cell");

            ESM::Cell *cell = new ESM::Cell (record);
            mStore.cells.intCells.insert (std::make_pair (record.mName, cell));
            return cell;
        }
        else
        {
            if (mStore.cells.searchExt (record.mData.mX, record.mData.mY))
                throw std::runtime_error ("failed creating exterior cell");

            ESM::Cell *cell = new ESM::Cell (record);
            mStore.cells.extCells.insert (
                std::make_pair (std::make_pair (record.mData.mX, record.mData.mY), cell));
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
        /// \todo split this function up into subfunctions

        mWorldScene->update (duration);
        
        float pitch, yaw;
        Ogre::Vector3 eyepos;
        mRendering->getPlayerData(eyepos, pitch, yaw);
        mPhysics->updatePlayerData(eyepos, pitch, yaw);

        mWeatherManager->update (duration);

        // inform the GUI about focused object
        try
        {
            MWWorld::Ptr object = getPtrViaHandle(mFacedHandle);
            MWBase::Environment::get().getWindowManager()->setFocusObject(object);

            // retrieve object dimensions so we know where to place the floating label
            Ogre::SceneNode* node = object.getRefData().getBaseNode();
            Ogre::AxisAlignedBox bounds;
            int i;
            for (i=0; i<node->numAttachedObjects(); ++i)
            {
                Ogre::MovableObject* ob = node->getAttachedObject(i);
                bounds.merge(ob->getWorldBoundingBox());
            }
            if (bounds.isFinite())
            {
                Vector4 screenCoords = mRendering->boundingBoxToScreen(bounds);
                MWBase::Environment::get().getWindowManager()->setFocusObjectScreenCoords(
                    screenCoords[0], screenCoords[1], screenCoords[2], screenCoords[3]);
            }
        }
        catch (std::runtime_error&)
        {
            MWWorld::Ptr null;
            MWBase::Environment::get().getWindowManager()->setFocusObject(null);
        }

        if (!mRendering->occlusionQuerySupported())
        {
            // cast a ray from player to sun to detect if the sun is visible
            // this is temporary until we find a better place to put this code
            // currently its here because we need to access the physics system
            float* p = mPlayer->getPlayer().getRefData().getPosition().pos;
            Vector3 sun = mRendering->getSkyManager()->getRealSunPos();
            sun = Vector3(sun.x, -sun.z, sun.y);
            mRendering->getSkyManager()->setGlare(!mPhysics->castRay(Ogre::Vector3(p[0], p[1], p[2]), sun));
        }

        // update faced handle (object the player is looking at)
        // this uses a mixture of raycasts and occlusion queries.
        else // if (mRendering->occlusionQuerySupported())
        {
            MWRender::OcclusionQuery* query = mRendering->getOcclusionQuery();
            if (!query->occlusionTestPending())
            {
                // get result of last query
                if (mNumFacing == 0) mFacedHandle = "";
                else if (mNumFacing == 1)
                {
                    bool result = query->getTestResult();
                    mFacedHandle = result ? mFaced1Name : "";
                }
                else if (mNumFacing == 2)
                {
                    bool result = query->getTestResult();
                    mFacedHandle = result ? mFaced2Name : mFaced1Name;
                }

                // send new query
                // figure out which object we want to test against
                std::vector < std::pair < float, std::string > > results;
                if (MWBase::Environment::get().getWindowManager()->isGuiMode())
                {
                    float x, y;
                    MWBase::Environment::get().getWindowManager()->getMousePosition(x, y);
                    results = mPhysics->getFacedObjects(x, y);
                }
                else
                    results = mPhysics->getFacedObjects();

                // ignore the player and other things we're not interested in
                std::vector < std::pair < float, std::string > >::iterator it = results.begin();
                while (it != results.end())
                {
                    if ( (*it).second.find("HeightField") != std::string::npos // not interested in terrain
                    || getPtrViaHandle((*it).second) == mPlayer->getPlayer() ) // not interested in player (unless you want to talk to yourself)
                    {
                        it = results.erase(it);
                    }
                    else
                        ++it;
                }

                if (results.size() == 0)
                {
                    mNumFacing = 0;
                }
                else if (results.size() == 1)
                {
                    mFaced1 = getPtrViaHandle(results.front().second);
                    mFaced1Name = results.front().second;
                    mNumFacing = 1;

                    btVector3 p;
                    if (MWBase::Environment::get().getWindowManager()->isGuiMode())
                    {
                        float x, y;
                        MWBase::Environment::get().getWindowManager()->getMousePosition(x, y);
                        p = mPhysics->getRayPoint(results.front().first, x, y);
                    }
                    else
                        p = mPhysics->getRayPoint(results.front().first);
                    Ogre::Vector3 pos(p.x(), p.z(), -p.y());
                    Ogre::SceneNode* node = mFaced1.getRefData().getBaseNode();

                    //std::cout << "Num facing 1 : " << mFaced1Name <<  std::endl;
                    //std::cout << "Type 1 " << mFaced1.getTypeName() <<  std::endl;

                    query->occlusionTest(pos, node);
                }
                else
                {
                    mFaced1Name = results.front().second;
                    mFaced2Name = results[1].second;
                    mFaced1 = getPtrViaHandle(results.front().second);
                    mFaced2 = getPtrViaHandle(results[1].second);
                    mNumFacing = 2;

                    btVector3 p;
                    if (MWBase::Environment::get().getWindowManager()->isGuiMode())
                    {
                        float x, y;
                        MWBase::Environment::get().getWindowManager()->getMousePosition(x, y);
                        p = mPhysics->getRayPoint(results[1].first, x, y);
                    }
                    else
                        p = mPhysics->getRayPoint(results[1].first);
                    Ogre::Vector3 pos(p.x(), p.z(), -p.y());
                    Ogre::SceneNode* node1 = mFaced1.getRefData().getBaseNode();
                    Ogre::SceneNode* node2 = mFaced2.getRefData().getBaseNode();

                    // no need to test if the first node is not occluder
                    if (!query->isPotentialOccluder(node1) && (mFaced1.getTypeName().find("Static") == std::string::npos))
                    {
                        mFacedHandle = mFaced1Name;
                        //std::cout << "node1 Not an occluder" << std::endl;
                        return;
                    }

                    // no need to test if the second object is static (thus cannot be activated)
                    if (mFaced2.getTypeName().find("Static") != std::string::npos)
                    {
                        mFacedHandle = mFaced1Name;
                        return;
                    }

                    // work around door problems
                    if (mFaced1.getTypeName().find("Static") != std::string::npos
                        && mFaced2.getTypeName().find("Door") != std::string::npos)
                    {
                        mFacedHandle = mFaced2Name;
                        return;
                    }

                    query->occlusionTest(pos, node2);
                }
            }
        }
    }

    bool World::isCellExterior() const
    {
        Ptr::CellStore *currentCell = mWorldScene->getCurrentCell();
        if (currentCell)
        {
            if (!(currentCell->cell->mData.mFlags & ESM::Cell::Interior))
                return true;
            else
                return false;
        }
        return false;
    }

    bool World::isCellQuasiExterior() const
    {
        Ptr::CellStore *currentCell = mWorldScene->getCurrentCell();
        if (currentCell)
        {
            if (!(currentCell->cell->mData.mFlags & ESM::Cell::QuasiEx))
                return false;
            else
                return true;
        }
        return false;
    }

    int World::getCurrentWeather() const
    {
        return mWeatherManager->getWeatherID();
    }

    void World::changeWeather(const std::string& region, const unsigned int id)
    {
        mWeatherManager->changeWeather(region, id);
    }

    OEngine::Render::Fader* World::getFader()
    {
        return mRendering->getFader();
    }

    Ogre::Vector2 World::getNorthVector (CellStore* cell)
    {
        MWWorld::CellRefList<ESM::Static>& statics = cell->statics;
        MWWorld::LiveCellRef<ESM::Static>* ref = statics.find("northmarker");
        if (!ref)
            return Vector2(0, 1);
        Ogre::SceneNode* node = ref->mData.getBaseNode();
        Vector3 dir = node->_getDerivedOrientation().yAxis();
        Vector2 d = Vector2(dir.x, dir.z);
        return d;
    }

    std::vector<World::DoorMarker> World::getDoorMarkers (CellStore* cell)
    {
        std::vector<World::DoorMarker> result;

        MWWorld::CellRefList<ESM::Door>& doors = cell->doors;
        std::list< MWWorld::LiveCellRef<ESM::Door> >& refList = doors.list;
        for (std::list< MWWorld::LiveCellRef<ESM::Door> >::iterator it = refList.begin(); it != refList.end(); ++it)
        {
            MWWorld::LiveCellRef<ESM::Door>& ref = *it;

            if (ref.ref.mTeleport)
            {
                World::DoorMarker newMarker;

                std::string dest;
                if (ref.ref.mDestCell != "")
                {
                    // door leads to an interior, use interior name
                    dest = ref.ref.mDestCell;
                }
                else
                {
                    // door leads to exterior, use cell name (if any), otherwise translated region name
                    int x,y;
                    positionToIndex (ref.ref.mDoorDest.pos[0], ref.ref.mDoorDest.pos[1], x, y);
                    const ESM::Cell* cell = mStore.cells.findExt(x,y);
                    if (cell->mName != "")
                        dest = cell->mName;
                    else
                    {
                        const ESM::Region* region = mStore.regions.search(cell->mRegion);
                        dest = region->mName;
                    }
                }

                newMarker.name = dest;

                ESM::Position pos = ref.mData.getPosition ();

                newMarker.x = pos.pos[0];
                newMarker.y = pos.pos[1];
                result.push_back(newMarker);
            }
        }

        return result;
    }

    void World::getInteriorMapPosition (Ogre::Vector2 position, float& nX, float& nY, int &x, int& y)
    {
        mRendering->getInteriorMapPosition(position, nX, nY, x, y);
    }

    bool World::isPositionExplored (float nX, float nY, int x, int y, bool interior)
    {
        return mRendering->isPositionExplored(nX, nY, x, y, interior);
    }

    void World::setWaterHeight(const float height)
    {
        mRendering->setWaterHeight(height);
    }

    void World::toggleWater()
    {
        mRendering->toggleWater();
    }

    bool World::placeObject (const Ptr& object, float cursorX, float cursorY)
    {
        std::pair<bool, Ogre::Vector3> result = mPhysics->castRay(cursorX, cursorY);

        if (!result.first)
            return false;

        CellStore* cell;
        if (isCellExterior())
        {
            int cellX, cellY;
            positionToIndex(result.second[0], -result.second[2], cellX, cellY);
            cell = mCells.getExterior(cellX, cellY);
        }
        else
            cell = getPlayer().getPlayer().getCell();

        ESM::Position pos = getPlayer().getPlayer().getRefData().getPosition();
        pos.pos[0] = result.second[0];
        pos.pos[1] = -result.second[2];
        pos.pos[2] = result.second[1];

        copyObjectToCell(object, *cell, pos);
        object.getRefData().setCount(0);

        return true;
    }

    bool World::canPlaceObject(float cursorX, float cursorY)
    {
        std::pair<bool, Ogre::Vector3> result = mPhysics->castRay(cursorX, cursorY);

        /// \todo also check if the wanted position is on a flat surface, and not e.g. against a vertical wall!

        if (!result.first)
            return false;
        return true;
    }

    void
    World::copyObjectToCell(const Ptr &object, CellStore &cell, const ESM::Position &pos)
    {
        /// \todo add searching correct cell for position specified
        MWWorld::Ptr dropped =
            MWWorld::Class::get(object).copyToCell(object, cell, pos);

        Ogre::Vector3 min, max;
        if (mPhysics->getObjectAABB(object, min, max)) {
            float *pos = dropped.getRefData().getPosition().pos;
            pos[0] -= (min.x + max.x) / 2;
            pos[1] -= (min.y + max.y) / 2;
            pos[2] -= min.z;
        }

        if (mWorldScene->isCellActive(cell)) {
            if (dropped.getRefData().isEnabled()) {
                mWorldScene->addObjectToScene(dropped);
            }
            std::string script = MWWorld::Class::get(dropped).getScript(dropped);
            if (!script.empty()) {
                mLocalScripts.add(script, dropped);
            }
        }
    }

    void World::dropObjectOnGround (const Ptr& object)
    {
        MWWorld::Ptr::CellStore* cell = getPlayer().getPlayer().getCell();

        ESM::Position pos =
            getPlayer().getPlayer().getRefData().getPosition();

        Ogre::Vector3 orig =
            Ogre::Vector3(pos.pos[0], pos.pos[1], pos.pos[2]);
        Ogre::Vector3 dir = Ogre::Vector3(0, 0, -1);

        float len = (pos.pos[2] >= 0) ? pos.pos[2] : -pos.pos[2];
        len += 100.0;

        std::pair<bool, Ogre::Vector3> hit =
            mPhysics->castRay(orig, dir, len);
        pos.pos[2] = hit.second.z;

        copyObjectToCell(object, *cell, pos);
        object.getRefData().setCount(0);
    }

    void World::processChangedSettings(const Settings::CategorySettingVector& settings)
    {
        mRendering->processChangedSettings(settings);
    }

    void World::getTriangleBatchCount(unsigned int &triangles, unsigned int &batches)
    {
        mRendering->getTriangleBatchCount(triangles, batches);
    }

    bool
    World::isSwimming(const MWWorld::Ptr &object)
    {
        /// \todo add check ifActor() - only actors can swim
        float *fpos = object.getRefData().getPosition().pos;
        Ogre::Vector3 pos(fpos[0], fpos[1], fpos[2]);

        /// \fixme should rely on object height
        pos.z += 30;

        return isUnderwater(*object.getCell()->cell, pos);
    }

    bool
    World::isUnderwater(const ESM::Cell &cell, const Ogre::Vector3 &pos)
    {
        if (!(cell.mData.mFlags & ESM::Cell::HasWater)) {
            return false;
        }
        return pos.z < cell.mWater;
    }

    void World::renderPlayer()
    {
        mRendering->renderPlayer(mPlayer->getPlayer());
    }

    void World::setupExternalRendering (MWRender::ExternalRendering& rendering)
    {
        mRendering->setupExternalRendering (rendering);
    }

    int World::canRest ()
    {
        Ptr::CellStore *currentCell = mWorldScene->getCurrentCell();

        Ogre::Vector3 playerPos;
        float* pos = mPlayer->getPlayer ().getRefData ().getPosition ().pos;
        playerPos.x = pos[0];
        playerPos.y = pos[1];
        playerPos.z = pos[2];

        std::pair<bool, Ogre::Vector3> hit =
                mPhysics->castRay(playerPos, Ogre::Vector3(0,0,-1), 50);
        bool isOnGround = (hit.first ? (hit.second.distance (playerPos) < 25) : false);

        if (!isOnGround || isUnderwater (*currentCell->cell, playerPos))
            return 2;

        if (currentCell->cell->mData.mFlags & ESM::Cell::NoSleep)
            return 1;

        return 0;

    }
}
