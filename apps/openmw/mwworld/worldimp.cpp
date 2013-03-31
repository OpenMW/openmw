#include "worldimp.hpp"

#include <libs/openengine/bullet/physic.hpp>

#include <components/bsa/bsa_archive.hpp>
#include <components/files/collections.hpp>
#include <components/compiler/locals.hpp>

#include "../mwbase/environment.hpp"
#include "../mwbase/soundmanager.hpp"
#include "../mwbase/mechanicsmanager.hpp"
#include "../mwbase/windowmanager.hpp"
#include "../mwbase/scriptmanager.hpp"

#include "../mwmechanics/creaturestats.hpp"

#include "../mwrender/sky.hpp"
#include "../mwrender/player.hpp"

#include "../mwclass/door.hpp"

#include "player.hpp"
#include "manualref.hpp"
#include "cellfunctors.hpp"
#include "containerstore.hpp"

using namespace Ogre;

namespace
{
/*  // NOTE this code is never instantiated (proper copy in localscripts.cpp),
    //      so this commented out to not produce syntactic errors

    template<typename T>
    void listCellScripts (const MWWorld::ESMStore& store,
        MWWorld::CellRefList<T>& cellRefList, MWWorld::LocalScripts& localScripts,
        MWWorld::Ptr::CellStore *cell)
    {
        for (typename MWWorld::CellRefList<T>::List::iterator iter (
            cellRefList.mList.begin());
            iter!=cellRefList.mList.end(); ++iter)
        {
            if (!iter->mBase->mScript.empty() && iter->mData.getCount())
            {
                if (const ESM::Script *script = store.get<ESM::Script>().find (iter->mBase->mScript))
                {
                    iter->mData.setLocals (*script);

                    localScripts.add (iter->mBase->mScript, MWWorld::Ptr (&*iter, cell));
                }
            }
        }
    }
*/
    template<typename T>
    MWWorld::LiveCellRef<T> *searchViaHandle (const std::string& handle,
        MWWorld::CellRefList<T>& refList)
    {
        typedef typename MWWorld::CellRefList<T>::List::iterator iterator;

        for (iterator iter (refList.mList.begin()); iter!=refList.mList.end(); ++iter)
        {
            if (iter->mData.getCount() > 0 && iter->mData.getBaseNode()){
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
            searchViaHandle (handle, cell.mActivators))
            return Ptr (ref, &cell);
        if (MWWorld::LiveCellRef<ESM::Potion> *ref = searchViaHandle (handle, cell.mPotions))
            return Ptr (ref, &cell);
        if (MWWorld::LiveCellRef<ESM::Apparatus> *ref = searchViaHandle (handle, cell.mAppas))
            return Ptr (ref, &cell);
        if (MWWorld::LiveCellRef<ESM::Armor> *ref = searchViaHandle (handle, cell.mArmors))
            return Ptr (ref, &cell);
        if (MWWorld::LiveCellRef<ESM::Book> *ref = searchViaHandle (handle, cell.mBooks))
            return Ptr (ref, &cell);
        if (MWWorld::LiveCellRef<ESM::Clothing> *ref = searchViaHandle (handle, cell.mClothes))
            return Ptr (ref, &cell);
        if (MWWorld::LiveCellRef<ESM::Container> *ref =
            searchViaHandle (handle, cell.mContainers))
            return Ptr (ref, &cell);
        if (MWWorld::LiveCellRef<ESM::Creature> *ref =
            searchViaHandle (handle, cell.mCreatures))
            return Ptr (ref, &cell);
        if (MWWorld::LiveCellRef<ESM::Door> *ref = searchViaHandle (handle, cell.mDoors))
            return Ptr (ref, &cell);
        if (MWWorld::LiveCellRef<ESM::Ingredient> *ref =
            searchViaHandle (handle, cell.mIngreds))
            return Ptr (ref, &cell);
        if (MWWorld::LiveCellRef<ESM::Light> *ref = searchViaHandle (handle, cell.mLights))
            return Ptr (ref, &cell);
        if (MWWorld::LiveCellRef<ESM::Lockpick> *ref = searchViaHandle (handle, cell.mLockpicks))
            return Ptr (ref, &cell);
        if (MWWorld::LiveCellRef<ESM::Miscellaneous> *ref = searchViaHandle (handle, cell.mMiscItems))
            return Ptr (ref, &cell);
        if (MWWorld::LiveCellRef<ESM::NPC> *ref = searchViaHandle (handle, cell.mNpcs))
            return Ptr (ref, &cell);
        if (MWWorld::LiveCellRef<ESM::Probe> *ref = searchViaHandle (handle, cell.mProbes))
            return Ptr (ref, &cell);
        if (MWWorld::LiveCellRef<ESM::Repair> *ref = searchViaHandle (handle, cell.mRepairs))
            return Ptr (ref, &cell);
        if (MWWorld::LiveCellRef<ESM::Static> *ref = searchViaHandle (handle, cell.mStatics))
            return Ptr (ref, &cell);
        if (MWWorld::LiveCellRef<ESM::Weapon> *ref = searchViaHandle (handle, cell.mWeapons))
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

    World::World (OEngine::Render::OgreRenderer& renderer,
        const Files::Collections& fileCollections,
        const std::vector<std::string>& master, const std::vector<std::string>& plugins,
	const boost::filesystem::path& resDir, const boost::filesystem::path& cacheDir, bool newGame,
        ToUTF8::Utf8Encoder* encoder, const std::map<std::string,std::string>& fallbackMap, int mActivationDistanceOverride)
    : mPlayer (0), mLocalScripts (mStore), mGlobalVariables (0),
      mSky (true), mCells (mStore, mEsm),
      mNumFacing(0), mActivationDistanceOverride (mActivationDistanceOverride),mFallback(fallbackMap)
    {
        mPhysics = new PhysicsSystem(renderer);
        mPhysEngine = mPhysics->getEngine();

        mRendering = new MWRender::RenderingManager(renderer, resDir, cacheDir, mPhysEngine,&mFallback);

        mPhysEngine->setSceneManager(renderer.getScene());

        mWeatherManager = new MWWorld::WeatherManager(mRendering,&mFallback);

        int idx = 0;
        // NOTE: We might need to reserve one more for the running game / save.
        mEsm.resize(master.size() + plugins.size());
        for (std::vector<std::string>::size_type i = 0; i < master.size(); i++, idx++)
        {
            boost::filesystem::path masterPath (fileCollections.getCollection (".esm").getPath (master[i]));

            std::cout << "Loading ESM " << masterPath.string() << "\n";

            // This parses the ESM file
            ESM::ESMReader lEsm;
            lEsm.setEncoder(encoder);
            lEsm.setIndex(idx);
            lEsm.setGlobalReaderList(&mEsm);
            lEsm.open (masterPath.string());
            mEsm[idx] = lEsm;
            mStore.load (mEsm[idx]);
        }

        for (std::vector<std::string>::size_type i = 0; i < plugins.size(); i++, idx++)
        {
            boost::filesystem::path pluginPath (fileCollections.getCollection (".esp").getPath (plugins[i]));

            std::cout << "Loading ESP " << pluginPath.string() << "\n";

            // This parses the ESP file
            ESM::ESMReader lEsm;
            lEsm.setEncoder(encoder);
            lEsm.setIndex(idx);
            lEsm.setGlobalReaderList(&mEsm);
            lEsm.open (pluginPath.string());
            mEsm[idx] = lEsm;
            mStore.load (mEsm[idx]);
        }

        mStore.setUp();

        mPlayer = new MWWorld::Player (mStore.get<ESM::NPC>().find ("player"), *this);
        mRendering->attachCameraTo(mPlayer->getPlayer());

        // global variables
        mGlobalVariables = new Globals (mStore);

        if (newGame)
        {
            // set new game mark
            mGlobalVariables->setInt ("chargenstate", 1);
        }

        mGlobalVariables->setInt ("pcrace", 3);

        mWorldScene = new Scene(*mRendering, mPhysics);

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
        const ESM::Cell *cell = mStore.get<ESM::Cell>().searchExtByName (cellName);
        if (cell != 0) {
            return cell;
        }

        // didn't work -> now check for regions
        const MWWorld::Store<ESM::Region> &regions = mStore.get<ESM::Region>();
        MWWorld::Store<ESM::Region>::iterator it = regions.begin();
        for (; it != regions.end(); ++it)
        {
            if (Misc::StringUtils::ciEqual(cellName, it->mName))
            {
                return mStore.get<ESM::Cell>().searchExtByRegion(it->mId);
            }
        }

        return 0;
    }

    const MWWorld::Fallback *World::getFallback() const
    {
        return &mFallback;
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

    const MWWorld::ESMStore& World::getStore() const
    {
        return mStore;
    }

    std::vector<ESM::ESMReader>& World::getEsmReader()
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

    std::vector<std::string> World::getGlobals () const
    {
        return mGlobalVariables->getGlobals();
    }

    std::string World::getCurrentCellName () const
    {
        std::string name;

        Ptr::CellStore *cell = mWorldScene->getCurrentCell();
        if (cell->mCell->isExterior())
        {
            if (cell->mCell->mName != "")
            {
                name = cell->mCell->mName;
            }
            else
            {
                const ESM::Region* region =
                    MWBase::Environment::get().getWorld()->getStore().get<ESM::Region>().search(cell->mCell->mRegion);
                if (region)
                    name = region->mName;
                else
                {
                    const ESM::GameSetting *setting =
                        MWBase::Environment::get().getWorld()->getStore().get<ESM::GameSetting>().search("sDefaultCellname");

                    if (setting && setting->mValue.getType()==ESM::VT_String)
                        name = setting->mValue.getString();
                }

            }
        }
        else
        {
            name = cell->mCell->mName;
        }

        return name;
    }

    void World::removeRefScript (MWWorld::RefData *ref)
    {
        mLocalScripts.remove (ref);
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
        Ptr res = searchPtrViaHandle (handle);
        if (res.isEmpty ())
            throw std::runtime_error ("unknown Ogre handle: " + handle);
        return res;
    }

    Ptr World::searchPtrViaHandle (const std::string& handle)
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

        return MWWorld::Ptr();
    }

    void World::addContainerScripts(const Ptr& reference, Ptr::CellStore * cell)
    {
        if( reference.getTypeName()==typeid (ESM::Container).name() ||
            reference.getTypeName()==typeid (ESM::NPC).name() ||
            reference.getTypeName()==typeid (ESM::Creature).name())
        {
            MWWorld::ContainerStore& container = MWWorld::Class::get(reference).getContainerStore(reference);
            for(MWWorld::ContainerStoreIterator it = container.begin(); it != container.end(); ++it)
            {
                std::string script = MWWorld::Class::get(*it).getScript(*it);
                if(script != "")
                {
                    MWWorld::Ptr item = *it;
                    item.mCell = cell;
                    mLocalScripts.add (script, item);
                }
            }
        }
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

    void World::removeContainerScripts(const Ptr& reference)
    {
        if( reference.getTypeName()==typeid (ESM::Container).name() ||
            reference.getTypeName()==typeid (ESM::NPC).name() ||
            reference.getTypeName()==typeid (ESM::Creature).name())
        {
            MWWorld::ContainerStore& container = MWWorld::Class::get(reference).getContainerStore(reference);
            for(MWWorld::ContainerStoreIterator it = container.begin(); it != container.end(); ++it)
            {
                std::string script = MWWorld::Class::get(*it).getScript(*it);
                if(script != "")
                {
                    MWWorld::Ptr item = *it;
                    mLocalScripts.remove (item);
                }
            }
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
        if (day<1)
            day = 1;

        int month = mGlobalVariables->getInt ("month");

        while (true)
        {
            int days = getDaysPerMonth (month);
            if (day<=days)
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

        if (mGlobalVariables->getInt ("day")>days)
            mGlobalVariables->setInt ("day", days);

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

    float World::getMaxActivationDistance ()
    {
        if (mActivationDistanceOverride >= 0)
            return mActivationDistanceOverride;

        return (std::max) (getNpcActivationDistance (), getObjectActivationDistance ());
    }

    float World::getNpcActivationDistance ()
    {
        if (mActivationDistanceOverride >= 0)
            return mActivationDistanceOverride;

        return getStore().get<ESM::GameSetting>().find ("iMaxActivateDist")->getInt()*5/4;
    }

    float World::getObjectActivationDistance ()
    {
        if (mActivationDistanceOverride >= 0)
            return mActivationDistanceOverride;

        return getStore().get<ESM::GameSetting>().find ("iMaxActivateDist")->getInt();
    }

    MWWorld::Ptr World::getFacedObject()
    {
        std::pair<float, std::string> result;

        if (!mRendering->occlusionQuerySupported())
            result = mPhysics->getFacedHandle (*this, getMaxActivationDistance ());
        else
            result = std::make_pair (mFacedDistance, mFacedHandle);

        if (result.second.empty())
            return MWWorld::Ptr ();

        MWWorld::Ptr object = searchPtrViaHandle (result.second);
        float ActivationDistance;

        if (MWBase::Environment::get().getWindowManager()->isConsoleMode())
            ActivationDistance = getObjectActivationDistance ()*50;
        else if (object.getTypeName ().find("NPC") != std::string::npos)
            ActivationDistance = getNpcActivationDistance ();
        else
            ActivationDistance = getObjectActivationDistance ();

        if (result.first > ActivationDistance)
            return MWWorld::Ptr ();

        return object;
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
                removeContainerScripts (ptr);
            }
        }
    }

    void World::moveObject(const Ptr &ptr, CellStore &newCell, float x, float y, float z)
    {
        ESM::Position &pos = ptr.getRefData().getPosition();
        pos.pos[0] = x;
        pos.pos[1] = y;
        pos.pos[2] = z;
        Ogre::Vector3 vec(x, y, z);

        CellStore *currCell = ptr.getCell();
        bool isPlayer = ptr == mPlayer->getPlayer();
        bool haveToMove = mWorldScene->isCellActive(*currCell) || isPlayer;

        if (*currCell != newCell)
        {
        removeContainerScripts(ptr);

            if (isPlayer)
            {
                if (!newCell.isExterior())
                    changeToInteriorCell(Misc::StringUtils::lowerCase(newCell.mCell->mName), pos);
                else
                {
                    int cellX = newCell.mCell->getGridX();
                    int cellY = newCell.mCell->getGridY();
                    mWorldScene->changeCell(cellX, cellY, pos, false);
                }
            }
            else
            {
                if (!mWorldScene->isCellActive(*currCell))
                    copyObjectToCell(ptr, newCell, pos);
                else if (!mWorldScene->isCellActive(newCell))
                {
                    MWWorld::Class::get(ptr)
                        .copyToCell(ptr, newCell)
                        .getRefData()
                        .setBaseNode(0);

                    mWorldScene->removeObjectFromScene(ptr);
                    mLocalScripts.remove(ptr);
                    removeContainerScripts (ptr);
                    haveToMove = false;
                }
                else
                {
                    MWWorld::Ptr copy =
                        MWWorld::Class::get(ptr).copyToCell(ptr, newCell);

                    mRendering->updateObjectCell(ptr, copy);

                    MWBase::MechanicsManager *mechMgr = MWBase::Environment::get().getMechanicsManager();
                    mechMgr->updateCell(ptr, copy);

                    std::string script =
                        MWWorld::Class::get(ptr).getScript(ptr);
                    if (!script.empty())
                    {
                        mLocalScripts.remove(ptr);
                        removeContainerScripts (ptr);
                        mLocalScripts.add(script, copy);
                        addContainerScripts (copy, &newCell);
                    }
                }
                ptr.getRefData().setCount(0);
            }
        }
        if (haveToMove)
        {
            mRendering->moveObject(ptr, vec);
            mPhysics->moveObject (ptr);
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

        if(ptr.getRefData().getBaseNode() == 0)
            return;
        mRendering->scaleObject(ptr, Vector3(scale,scale,scale));
        mPhysics->scaleObject(ptr);
    }

    void World::rotateObject (const Ptr& ptr,float x,float y,float z, bool adjust)
    {
        Ogre::Vector3 rot;
        rot.x = Ogre::Degree(x).valueRadians();
        rot.y = Ogre::Degree(y).valueRadians();
        rot.z = Ogre::Degree(z).valueRadians();

        if (mRendering->rotateObject(ptr, rot, adjust))
        {
            // rotate physically iff renderer confirm so
            float *objRot = ptr.getRefData().getPosition().rot;
            objRot[0] = rot.x, objRot[1] = rot.y, objRot[2] = rot.z;

            if (ptr.getRefData().getBaseNode() != 0) {
                mPhysics->rotateObject(ptr);
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

    void World::doPhysics(const PtrMovementList &actors, float duration)
    {
        /* No duration? Shouldn't be any movement, then. */
        if(duration <= 0.0f)
            return;

        PtrMovementList::const_iterator player(actors.end());
        for(PtrMovementList::const_iterator iter(actors.begin());iter != actors.end();iter++)
        {
            if(iter->first.getRefData().getHandle() == "player")
            {
                /* Handle player last, in case a cell transition occurs */
                player = iter;
                continue;
            }
            Ogre::Vector3 vec = mPhysics->move(iter->first, iter->second, duration,
                                               !isSwimming(iter->first) && !isFlying(iter->first));
            moveObjectImp(iter->first, vec.x, vec.y, vec.z);
        }
        if(player != actors.end())
        {
            Ogre::Vector3 vec = mPhysics->move(player->first, player->second, duration,
                                               !isSwimming(player->first) && !isFlying(player->first));
            moveObjectImp(player->first, vec.x, vec.y, vec.z);
        }
        // the only purpose this has currently is to update the debug drawer
        mPhysEngine->stepSimulation (duration);
    }

    bool World::toggleCollisionMode()
    {
        return mPhysics->toggleCollisionMode();;
    }

    bool World::toggleRenderMode (RenderMode mode)
    {
        return mRendering->toggleRenderMode (mode);
    }

    const ESM::Potion *World::createRecord (const ESM::Potion& record)
    {
        return mStore.insert(record);
    }

    const ESM::Class *World::createRecord (const ESM::Class& record)
    {
        return mStore.insert(record);
    }

    const ESM::Spell *World::createRecord (const ESM::Spell& record)
    {
        return mStore.insert(record);
    }

    const ESM::Cell *World::createRecord (const ESM::Cell& record)
    {
        return mStore.insert(record);
    }

    const ESM::NPC *World::createRecord(const ESM::NPC &record)
    {
        bool update = false;

        if (Misc::StringUtils::ciEqual(record.mId, "player"))
        {
            std::vector<std::string> ids;
            getStore().get<ESM::Race>().listIdentifier(ids);

            unsigned int i=0;

            for (; i<ids.size(); ++i)
                if (Misc::StringUtils::ciEqual (ids[i], record.mRace))
                    break;

            mGlobalVariables->setInt ("pcrace", (i == ids.size()) ? 0 : i+1);

            const ESM::NPC *player =
                mPlayer->getPlayer().get<ESM::NPC>()->mBase;

            update = record.isMale() != player->isMale() ||
                     !Misc::StringUtils::ciEqual(record.mRace, player->mRace) ||
                     !Misc::StringUtils::ciEqual(record.mHead, player->mHead) ||
                     !Misc::StringUtils::ciEqual(record.mHair, player->mHair);
        }
        const ESM::NPC *ret = mStore.insert(record);
        if (update) {
            mRendering->renderPlayer(mPlayer->getPlayer());
        }
        return ret;
    }

    void World::update (float duration, bool paused)
    {
        mWeatherManager->update (duration);

        mWorldScene->update (duration, paused);

        float pitch, yaw;
        Ogre::Vector3 eyepos;
        mRendering->getPlayerData(eyepos, pitch, yaw);
        mPhysics->updatePlayerData(eyepos, pitch, yaw);

        performUpdateSceneQueries ();

        updateWindowManager ();
    }

    void World::updateWindowManager ()
    {
        // inform the GUI about focused object
        MWWorld::Ptr object = getFacedObject ();

        MWBase::Environment::get().getWindowManager()->setFocusObject(object);

        // retrieve object dimensions so we know where to place the floating label
        if (!object.isEmpty ())
        {
            Ogre::SceneNode* node = object.getRefData().getBaseNode();
            Ogre::AxisAlignedBox bounds = node->_getWorldAABB();
            if (bounds.isFinite())
            {
                Vector4 screenCoords = mRendering->boundingBoxToScreen(bounds);
                MWBase::Environment::get().getWindowManager()->setFocusObjectScreenCoords(
                    screenCoords[0], screenCoords[1], screenCoords[2], screenCoords[3]);
            }
        }
    }

    void World::performUpdateSceneQueries ()
    {
        if (!mRendering->occlusionQuerySupported())
        {
            // cast a ray from player to sun to detect if the sun is visible
            // this is temporary until we find a better place to put this code
            // currently its here because we need to access the physics system
            float* p = mPlayer->getPlayer().getRefData().getPosition().pos;
            Vector3 sun = mRendering->getSkyManager()->getRealSunPos();
            mRendering->getSkyManager()->setGlare(!mPhysics->castRay(Ogre::Vector3(p[0], p[1], p[2]), sun));
        }

        updateFacedHandle ();
    }

    void World::updateFacedHandle ()
    {
        // send new query
        // figure out which object we want to test against
        std::vector < std::pair < float, std::string > > results;
        if (MWBase::Environment::get().getWindowManager()->isGuiMode())
        {
            float x, y;
            MWBase::Environment::get().getWindowManager()->getMousePosition(x, y);
            results = mPhysics->getFacedHandles(x, y, getMaxActivationDistance ());
            if (MWBase::Environment::get().getWindowManager()->isConsoleMode())
                results = mPhysics->getFacedHandles(x, y, getMaxActivationDistance ()*50);
        }
        else
        {
            results = mPhysics->getFacedHandles(getMaxActivationDistance ());
        }

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
            mFacedHandle = "";
            mFacedDistance = FLT_MAX;
        }
        else
        {
            mFacedHandle = results.front().second;
            mFacedDistance = results.front().first;
        }
    }

    bool World::isCellExterior() const
    {
        Ptr::CellStore *currentCell = mWorldScene->getCurrentCell();
        if (currentCell)
        {
            return currentCell->mCell->isExterior();
        }
        return false;
    }

    bool World::isCellQuasiExterior() const
    {
        Ptr::CellStore *currentCell = mWorldScene->getCurrentCell();
        if (currentCell)
        {
            if (!(currentCell->mCell->mData.mFlags & ESM::Cell::QuasiEx))
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
        MWWorld::CellRefList<ESM::Static>& statics = cell->mStatics;
        MWWorld::LiveCellRef<ESM::Static>* ref = statics.find("northmarker");
        if (!ref)
            return Vector2(0, 1);
        Ogre::SceneNode* node = ref->mData.getBaseNode();
        Vector3 dir = node->_getDerivedOrientation() * Ogre::Vector3(0,1,0);
        Vector2 d = Vector2(dir.x, dir.y);
        return d;
    }

    std::vector<World::DoorMarker> World::getDoorMarkers (CellStore* cell)
    {
        std::vector<World::DoorMarker> result;

        MWWorld::CellRefList<ESM::Door>& doors = cell->mDoors;
        CellRefList<ESM::Door>::List& refList = doors.mList;
        for (CellRefList<ESM::Door>::List::iterator it = refList.begin(); it != refList.end(); ++it)
        {
            MWWorld::LiveCellRef<ESM::Door>& ref = *it;

            if (ref.mRef.mTeleport)
            {
                World::DoorMarker newMarker;
                newMarker.name = MWClass::Door::getDestination(ref);

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

    void World::PCDropped (const Ptr& item)
    {
        std::string script = MWWorld::Class::get(item).getScript(item);

        // Set OnPCDrop Variable on item's script, if it has a script with that variable declared
        if(script != "")
            item.mRefData->getLocals().setVarByInt(script, "onpcdrop", 1);
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
            positionToIndex(result.second[0], result.second[1], cellX, cellY);
            cell = mCells.getExterior(cellX, cellY);
        }
        else
            cell = getPlayer().getPlayer().getCell();

        ESM::Position pos = getPlayer().getPlayer().getRefData().getPosition();
        pos.pos[0] = result.second[0];
        pos.pos[1] = result.second[1];
        pos.pos[2] = result.second[2];
        // We want only the Z part of the player's rotation
        pos.rot[0] = 0;
        pos.rot[1] = 0;

        Ptr dropped = copyObjectToCell(object, *cell, pos);
        PCDropped(dropped);
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


    Ptr World::copyObjectToCell(const Ptr &object, CellStore &cell, const ESM::Position &pos)
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
            addContainerScripts(dropped, &cell);
        }

        return dropped;
    }

    void World::dropObjectOnGround (const Ptr& actor, const Ptr& object)
    {
        MWWorld::Ptr::CellStore* cell = actor.getCell();

        ESM::Position pos =
            actor.getRefData().getPosition();
        // We want only the Z part of the actor's rotation
        pos.rot[0] = 0;
        pos.rot[1] = 0;

        Ogre::Vector3 orig =
            Ogre::Vector3(pos.pos[0], pos.pos[1], pos.pos[2]);
        Ogre::Vector3 dir = Ogre::Vector3(0, 0, -1);

        float len = (pos.pos[2] >= 0) ? pos.pos[2] : -pos.pos[2];
        len += 100.0;

        std::pair<bool, Ogre::Vector3> hit =
            mPhysics->castRay(orig, dir, len);
        pos.pos[2] = hit.second.z;

        Ptr dropped = copyObjectToCell(object, *cell, pos);
        if(actor == mPlayer->getPlayer()) // Only call if dropped by player
            PCDropped(dropped);
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
    World::isFlying(const MWWorld::Ptr &ptr) const
    {
        const MWWorld::Class &cls = MWWorld::Class::get(ptr);
        if(cls.isActor() && cls.getCreatureStats(ptr).getMagicEffects().get(MWMechanics::EffectKey(ESM::MagicEffect::Levitate)).mMagnitude > 0)
            return true;
        return false;
    }

    bool
    World::isSwimming(const MWWorld::Ptr &object) const
    {
        /// \todo add check ifActor() - only actors can swim
        float *fpos = object.getRefData().getPosition().pos;
        Ogre::Vector3 pos(fpos[0], fpos[1], fpos[2]);

        /// \fixme 3/4ths submerged?
        const OEngine::Physic::PhysicActor *actor = mPhysEngine->getCharacter(object.getRefData().getHandle());
        if(actor) pos.z += actor->getHalfExtents().z * 1.5;

        return isUnderwater(object.getCell(), pos);
    }

    bool
    World::isUnderwater(const MWWorld::Ptr::CellStore* cell, const Ogre::Vector3 &pos) const
    {
        if (!(cell->mCell->mData.mFlags & ESM::Cell::HasWater)) {
            return false;
        }
        return pos.z < cell->mWaterLevel;
    }

    bool World::isOnGround(const MWWorld::Ptr &ptr) const
    {
        RefData &refdata = ptr.getRefData();
        const OEngine::Physic::PhysicActor *physactor = mPhysEngine->getCharacter(refdata.getHandle());
        return physactor && physactor->getOnGround();
    }

    void World::renderPlayer()
    {
        mRendering->renderPlayer(mPlayer->getPlayer());
        mPhysics->addActor(mPlayer->getPlayer());
    }

    void World::setupExternalRendering (MWRender::ExternalRendering& rendering)
    {
        mRendering->setupExternalRendering (rendering);
    }

    int World::canRest ()
    {
        Ptr::CellStore *currentCell = mWorldScene->getCurrentCell();

        RefData &refdata = mPlayer->getPlayer().getRefData();
        Ogre::Vector3 playerPos(refdata.getPosition().pos);

        const OEngine::Physic::PhysicActor *physactor = mPhysEngine->getCharacter(refdata.getHandle());
        if(!physactor->getOnGround() || isUnderwater(currentCell, playerPos))
            return 2;
        if((currentCell->mCell->mData.mFlags&ESM::Cell::NoSleep))
            return 1;

        return 0;
    }

    MWRender::Animation* World::getAnimation(const MWWorld::Ptr &ptr)
    {
        return mRendering->getAnimation(ptr);
    }

    void World::playVideo (const std::string &name, bool allowSkipping)
    {
        mRendering->playVideo(name, allowSkipping);
    }

    void World::stopVideo ()
    {
        mRendering->stopVideo();
    }

    void World::frameStarted (float dt)
    {
        mRendering->frameStarted(dt);
    }
}
