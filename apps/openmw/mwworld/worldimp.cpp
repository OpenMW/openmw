#include "worldimp.hpp"

#include <libs/openengine/bullet/physic.hpp>

#include <components/bsa/bsa_archive.hpp>
#include <components/files/collections.hpp>
#include <components/compiler/locals.hpp>

#include <boost/math/special_functions/sign.hpp>

#include "../mwbase/environment.hpp"
#include "../mwbase/soundmanager.hpp"
#include "../mwbase/mechanicsmanager.hpp"
#include "../mwbase/windowmanager.hpp"
#include "../mwbase/scriptmanager.hpp"

#include "../mwmechanics/creaturestats.hpp"
#include "../mwmechanics/movement.hpp"

#include "../mwrender/sky.hpp"
#include "../mwrender/animation.hpp"

#include "../mwclass/door.hpp"

#include "player.hpp"
#include "manualref.hpp"
#include "cellfunctors.hpp"
#include "containerstore.hpp"
#include "inventorystore.hpp"

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
        const boost::filesystem::path& resDir, const boost::filesystem::path& cacheDir,
        ToUTF8::Utf8Encoder* encoder, const std::map<std::string,std::string>& fallbackMap, int mActivationDistanceOverride)
    : mPlayer (0), mLocalScripts (mStore), mGlobalVariables (0),
      mSky (true), mCells (mStore, mEsm),
      mNumFacing(0), mActivationDistanceOverride (mActivationDistanceOverride),
      mFallback(fallbackMap), mPlayIntro(0)
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

        // insert records that may not be present in all versions of MW
        if (mEsm[0].getFormat() == 0)
            ensureNeededRecords();

        mStore.movePlayerRecord();
        mStore.setUp();

        mGlobalVariables = new Globals (mStore);

        mWorldScene = new Scene(*mRendering, mPhysics);
    }

    void World::startNewGame()
    {
        mWorldScene->changeToVoid();

        mStore.clearDynamic();
        mStore.setUp();

        mCells.clear();

        // Rebuild player
        setupPlayer();
        const ESM::NPC* playerNpc = mStore.get<ESM::NPC>().find("player");
        MWWorld::Ptr player = mPlayer->getPlayer();
        renderPlayer();
        mRendering->resetCamera();

        // removes NpcStats, ContainerStore etc
        player.getRefData().setCustomData(NULL);

        // make sure to do this so that local scripts from items that were in the players inventory are removed
        mLocalScripts.clear();

        MWWorld::Class::get(player).getContainerStore(player).fill(playerNpc->mInventory, "", mStore);
        MWWorld::Class::get(player).getInventoryStore(player).autoEquip(player);

        MWBase::Environment::get().getWindowManager()->updatePlayer();

        ESM::Position pos;
        const int cellSize = 8192;
        pos.pos[0] = cellSize/2;
        pos.pos[1] = cellSize/2;
        pos.pos[2] = 0;
        pos.rot[0] = 0;
        pos.rot[1] = 0;
        pos.rot[2] = 0;
        mWorldScene->changeToExteriorCell(pos);


        // enable collision
        if(!mPhysics->toggleCollisionMode())
            mPhysics->toggleCollisionMode();

        // FIXME: should be set to 1, but the sound manager won't pause newly started sounds
        mPlayIntro = 2;

        // global variables
        delete mGlobalVariables;
        mGlobalVariables = new Globals (mStore);

        // set new game mark
        mGlobalVariables->setInt ("chargenstate", 1);
        mGlobalVariables->setInt ("pcrace", 3);

        // we don't want old weather to persist on a new game
        delete mWeatherManager;
        mWeatherManager = new MWWorld::WeatherManager(mRendering,&mFallback);

        MWBase::Environment::get().getScriptManager()->resetGlobalScripts();
    }


    void World::ensureNeededRecords()
    {
        if (!mStore.get<ESM::GameSetting>().search("sCompanionShare"))
        {
            ESM::GameSetting sCompanionShare;
            sCompanionShare.mId = "sCompanionShare";
            ESM::Variant value;
            value.setType(ESM::VT_String);
            value.setString("Companion Share");
            sCompanionShare.mValue = value;
            mStore.insertStatic(sCompanionShare);
        }
        if (!mStore.get<ESM::Global>().search("dayspassed"))
        {
            // vanilla Morrowind does not define dayspassed.
            ESM::Global dayspassed;
            dayspassed.mId = "dayspassed";
            ESM::Variant value;
            value.setType(ESM::VT_Long);
            value.setInteger(1); // but the addons start counting at 1 :(
            dayspassed.mValue = value;
            mStore.insertStatic(dayspassed);
        }
        if (!mStore.get<ESM::GameSetting>().search("fWereWolfRunMult"))
        {
            ESM::GameSetting fWereWolfRunMult;
            fWereWolfRunMult.mId = "fWereWolfRunMult";
            ESM::Variant value;
            value.setType(ESM::VT_Float);
            value.setFloat(1.f);
            fWereWolfRunMult.mValue = value;
            mStore.insertStatic(fWereWolfRunMult);
        }
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

    MWWorld::Ptr World::getFacedObject(const MWWorld::Ptr &ptr, float distance)
    {
        const ESM::Position &posdata = ptr.getRefData().getPosition();
        Ogre::Vector3 pos(posdata.pos);
        Ogre::Quaternion rot = Ogre::Quaternion(Ogre::Radian(posdata.rot[2]), Ogre::Vector3::NEGATIVE_UNIT_Z) *
                               Ogre::Quaternion(Ogre::Radian(posdata.rot[0]), Ogre::Vector3::UNIT_X);

        MWRender::Animation *anim = mRendering->getAnimation(ptr);
        if(anim != NULL)
        {
            Ogre::Node *node = anim->getNode("Head");
            if(node != NULL)
                pos += node->_getDerivedPosition();
        }

        std::pair<std::string,float> result = mPhysics->getFacedHandle(pos, rot, distance);
        if(result.first.empty())
            return MWWorld::Ptr();

        return searchPtrViaHandle(result.first);
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
        bool haveToMove = isPlayer || mWorldScene->isCellActive(*currCell);

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
                        MWWorld::Class::get(ptr).copyToCell(ptr, newCell, pos);

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
        ptr.getCellRef().mScale = scale;
        MWWorld::Class::get(ptr).adjustScale(ptr,scale);

        if(ptr.getRefData().getBaseNode() == 0)
            return;
        mRendering->scaleObject(ptr, Vector3(scale,scale,scale));
        mPhysics->scaleObject(ptr);
    }

    void World::rotateObjectImp (const Ptr& ptr, Ogre::Vector3 rot, bool adjust)
    {
        const float two_pi = Ogre::Math::TWO_PI;
        const float pi = Ogre::Math::PI;

        float *objRot = ptr.getRefData().getPosition().rot;
        if(adjust)
        {
            objRot[0] += rot.x;
            objRot[1] += rot.y;
            objRot[2] += rot.z;
        }
        else
        {
            objRot[0] = rot.x;
            objRot[1] = rot.y;
            objRot[2] = rot.z;
        }

        if(Class::get(ptr).isActor())
        {
            /* HACK? Actors shouldn't really be rotating around X (or Y), but
             * currently it's done so for rotating the camera, which needs
             * clamping.
             */
            const float half_pi = Ogre::Math::HALF_PI;

            if(objRot[0] < -half_pi)     objRot[0] = -half_pi;
            else if(objRot[0] > half_pi) objRot[0] =  half_pi;
        }
        else
        {
            while(objRot[0] < -pi) objRot[0] += two_pi;
            while(objRot[0] >  pi) objRot[0] -= two_pi;
        }

        while(objRot[1] < -pi) objRot[1] += two_pi;
        while(objRot[1] >  pi) objRot[1] -= two_pi;

        while(objRot[2] < -pi) objRot[2] += two_pi;
        while(objRot[2] >  pi) objRot[2] -= two_pi;

        if(ptr.getRefData().getBaseNode() != 0)
        {
            mRendering->rotateObject(ptr);
            mPhysics->rotateObject(ptr);
        }
    }

    void World::localRotateObject (const Ptr& ptr, float x, float y, float z)
    {
        if (ptr.getRefData().getBaseNode() != 0) {

            ptr.getRefData().getLocalRotation().rot[0]=Ogre::Degree(x).valueRadians();
            ptr.getRefData().getLocalRotation().rot[1]=Ogre::Degree(y).valueRadians();
            ptr.getRefData().getLocalRotation().rot[2]=Ogre::Degree(z).valueRadians();

            float fullRotateRad=Ogre::Degree(360).valueRadians();

            while(ptr.getRefData().getLocalRotation().rot[0]>=fullRotateRad)
                ptr.getRefData().getLocalRotation().rot[0]-=fullRotateRad;
            while(ptr.getRefData().getLocalRotation().rot[1]>=fullRotateRad)
                ptr.getRefData().getLocalRotation().rot[1]-=fullRotateRad;
            while(ptr.getRefData().getLocalRotation().rot[2]>=fullRotateRad)
                ptr.getRefData().getLocalRotation().rot[2]-=fullRotateRad;

            while(ptr.getRefData().getLocalRotation().rot[0]<=-fullRotateRad)
                ptr.getRefData().getLocalRotation().rot[0]+=fullRotateRad;
            while(ptr.getRefData().getLocalRotation().rot[1]<=-fullRotateRad)
                ptr.getRefData().getLocalRotation().rot[1]+=fullRotateRad;
            while(ptr.getRefData().getLocalRotation().rot[2]<=-fullRotateRad)
                ptr.getRefData().getLocalRotation().rot[2]+=fullRotateRad;

            float *worldRot = ptr.getRefData().getPosition().rot;

            Ogre::Quaternion worldRotQuat(Ogre::Quaternion(Ogre::Radian(-worldRot[0]), Ogre::Vector3::UNIT_X)*
            Ogre::Quaternion(Ogre::Radian(-worldRot[1]), Ogre::Vector3::UNIT_Y)*
            Ogre::Quaternion(Ogre::Radian(-worldRot[2]), Ogre::Vector3::UNIT_Z));

            Ogre::Quaternion rot(Ogre::Quaternion(Ogre::Radian(Ogre::Degree(-x).valueRadians()), Ogre::Vector3::UNIT_X)*
            Ogre::Quaternion(Ogre::Radian(Ogre::Degree(-y).valueRadians()), Ogre::Vector3::UNIT_Y)*
            Ogre::Quaternion(Ogre::Radian(Ogre::Degree(-z).valueRadians()), Ogre::Vector3::UNIT_Z));

            ptr.getRefData().getBaseNode()->setOrientation(worldRotQuat*rot);
            mPhysics->rotateObject(ptr);
        }
    }

    void World::adjustPosition(const Ptr &ptr)
    {
        Ogre::Vector3 pos (ptr.getRefData().getPosition().pos[0], ptr.getRefData().getPosition().pos[1], ptr.getRefData().getPosition().pos[2]);

        if(!ptr.getRefData().getBaseNode())
        {
            // will be adjusted when Ptr's cell becomes active
            return;
        }

        float terrainHeight = mRendering->getTerrainHeightAt(pos);

        if (pos.z < terrainHeight)
            pos.z = terrainHeight;

        ptr.getRefData().getPosition().pos[2] = pos.z + 20; // place slightly above. will snap down to ground with code below

        if (!isFlying(ptr))
        {
            Ogre::Vector3 traced = mPhysics->traceDown(ptr);
            if (traced.z < pos.z)
                pos.z = traced.z;
        }

        moveObject(ptr, *ptr.getCell(), pos.x, pos.y, pos.z);
    }

    void World::rotateObject (const Ptr& ptr,float x,float y,float z, bool adjust)
    {
        rotateObjectImp(ptr, Ogre::Vector3(Ogre::Degree(x).valueRadians(),
                                           Ogre::Degree(y).valueRadians(),
                                           Ogre::Degree(z).valueRadians()),
                        adjust);
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

        processDoors(duration);

        PtrMovementList::const_iterator player(actors.end());
        for(PtrMovementList::const_iterator iter(actors.begin());iter != actors.end();iter++)
        {
            if(iter->first.getRefData().getHandle() == "player")
            {
                /* Handle player last, in case a cell transition occurs */
                player = iter;
                continue;
            }

            rotateObjectImp(iter->first, Ogre::Vector3(iter->second.mRotation), true);

            Ogre::Vector3 vec = mPhysics->move(iter->first, Ogre::Vector3(iter->second.mPosition), duration,
                                               !isSwimming(iter->first) && !isFlying(iter->first));
            moveObjectImp(iter->first, vec.x, vec.y, vec.z);
        }
        if(player != actors.end())
        {
            rotateObjectImp(player->first, Ogre::Vector3(player->second.mRotation), true);

            Ogre::Vector3 vec = mPhysics->move(player->first, Ogre::Vector3(player->second.mPosition), duration,
                                               !isSwimming(player->first) && !isFlying(player->first));
            moveObjectImp(player->first, vec.x, vec.y, vec.z);
        }

        mPhysEngine->stepSimulation (duration);
    }

    bool World::castRay (float x1, float y1, float z1, float x2, float y2, float z2)
    {
        Ogre::Vector3 a(x1,y1,z1);std::cout << x1 << " " << x2;
        Ogre::Vector3 b(x2,y2,z2);
        return mPhysics->castRay(a,b,false,true);
    }

    void World::processDoors(float duration)
    {
        std::map<MWWorld::Ptr, int>::iterator it = mDoorStates.begin();
        while (it != mDoorStates.end())
        {
            if (!mWorldScene->isCellActive(*it->first.getCell()))
                mDoorStates.erase(it++);
            else
            {
                float oldRot = Ogre::Radian(it->first.getRefData().getLocalRotation().rot[2]).valueDegrees();
                float diff = duration * 90;
                float targetRot = std::min(std::max(0.f, oldRot + diff * (it->second ? 1 : -1)), 90.f);
                localRotateObject(it->first, 0, 0, targetRot);

                /// \todo should use convexSweepTest here
                std::vector<std::string> collisions = mPhysics->getCollisions(it->first);
                for (std::vector<std::string>::iterator cit = collisions.begin(); cit != collisions.end(); ++cit)
                {
                    MWWorld::Ptr ptr = getPtrViaHandle(*cit);
                    if (MWWorld::Class::get(ptr).isActor())
                    {
                        // we collided with an actor, we need to undo the rotation
                        localRotateObject(it->first, 0, 0, oldRot);
                        break;
                    }
                }

                if ((targetRot == 90.f && it->second) || targetRot == 0.f)
                    mDoorStates.erase(it++);
                else
                    ++it;
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

    const ESM::Armor *World::createRecord (const ESM::Armor& record)
    {
        return mStore.insert(record);
    }

    const ESM::Weapon *World::createRecord (const ESM::Weapon& record)
    {
        return mStore.insert(record);
    }

    const ESM::Clothing *World::createRecord (const ESM::Clothing& record)
    {
        return mStore.insert(record);
    }

    const ESM::Enchantment *World::createRecord (const ESM::Enchantment& record)
    {
        return mStore.insert(record);
    }

    const ESM::Book *World::createRecord (const ESM::Book& record)
    {
        return mStore.insert(record);
    }

    void World::update (float duration, bool paused)
    {
        if (mPlayIntro)
        {
            --mPlayIntro;
            if (mPlayIntro == 0)
                mRendering->playVideo(mFallback.getFallbackString("Movies_New_Game"), true);
        }

        mWeatherManager->update (duration);

        mWorldScene->update (duration, paused);

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

    bool World::vanityRotateCamera(float * rot)
    {
        return mRendering->vanityRotateCamera(rot);
    }

    void World::setCameraDistance(float dist, bool adjust, bool override)
    {
        return mRendering->setCameraDistance(dist, adjust, override);;
    }

    void World::setupPlayer()
    {
        const ESM::NPC *player = mStore.get<ESM::NPC>().find("player");
        if (!mPlayer)
            mPlayer = new MWWorld::Player(player, *this);
        else
            mPlayer->set(player);

        Ptr ptr = mPlayer->getPlayer();
        mRendering->setupPlayer(ptr);
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
        if((!physactor->getOnGround()&&physactor->getCollisionMode()) || isUnderwater(currentCell, playerPos))
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

    void World::activateDoor(const MWWorld::Ptr& door)
    {
        if (mDoorStates.find(door) != mDoorStates.end())
        {
            // if currently opening, then close, if closing, then open
            mDoorStates[door] = !mDoorStates[door];
        }
        else
        {
            if (door.getRefData().getLocalRotation().rot[2] == 0)
                mDoorStates[door] = 1; // open
            else
                mDoorStates[door] = 0; // close
        }
    }

    bool World::getOpenOrCloseDoor(const Ptr &door)
    {
        if (mDoorStates.find(door) != mDoorStates.end())
            return !mDoorStates[door]; // if currently opening or closing, then do the opposite
        return door.getRefData().getLocalRotation().rot[2] == 0;
    }

    bool World::getPlayerStandingOn (const MWWorld::Ptr& object)
    {
        MWWorld::Ptr player = mPlayer->getPlayer();
        if (!mPhysEngine->getCharacter("player")->getOnGround())
            return false;
        btVector3 from (player.getRefData().getPosition().pos[0], player.getRefData().getPosition().pos[1], player.getRefData().getPosition().pos[2]);
        btVector3 to = from - btVector3(0,0,5);
        std::pair<std::string, float> result = mPhysEngine->rayTest(from, to);
        return result.first == object.getRefData().getBaseNode()->getName();
    }

    bool World::getActorStandingOn (const MWWorld::Ptr& object)
    {
        return mPhysEngine->isAnyActorStandingOn(object.getRefData().getBaseNode()->getName());
    }

    float World::getWindSpeed()
    {
        if (isCellExterior() || isCellQuasiExterior())
            return mWeatherManager->getWindSpeed();
        else
            return 0.f;
    }

    void World::getContainersOwnedBy (const MWWorld::Ptr& npc, std::vector<MWWorld::Ptr>& out)
    {
        std::string refId = npc.getCellRef().mRefID;

        const Scene::CellStoreCollection& collection = mWorldScene->getActiveCells();
        for (Scene::CellStoreCollection::const_iterator cellIt = collection.begin(); cellIt != collection.end(); ++cellIt)
        {
            MWWorld::CellRefList<ESM::Container>& containers = (*cellIt)->mContainers;
            CellRefList<ESM::Container>::List& refList = containers.mList;
            for (CellRefList<ESM::Container>::List::iterator container = refList.begin(); container != refList.end(); ++container)
            {
                MWWorld::Ptr ptr (&*container, *cellIt);
                if (Misc::StringUtils::ciEqual(ptr.getCellRef().mOwner, npc.getCellRef().mRefID))
                    out.push_back(ptr);
            }
        }
    }

    struct ListHandlesFunctor
    {
        std::vector<std::string> mHandles;

        bool operator() (ESM::CellRef& ref, RefData& data)
        {
            Ogre::SceneNode* handle = data.getBaseNode();
            if (handle)
                mHandles.push_back(handle->getName());
            return true;
        }
    };

    void World::getItemsOwnedBy (const MWWorld::Ptr& npc, std::vector<MWWorld::Ptr>& out)
    {
        const Scene::CellStoreCollection& collection = mWorldScene->getActiveCells();
        for (Scene::CellStoreCollection::const_iterator cellIt = collection.begin(); cellIt != collection.end(); ++cellIt)
        {
            ListHandlesFunctor functor;
            (*cellIt)->forEach<ListHandlesFunctor>(functor);

            for (std::vector<std::string>::iterator it = functor.mHandles.begin(); it != functor.mHandles.end(); ++it)
                if (Misc::StringUtils::ciEqual(searchPtrViaHandle(*it).mCellRef->mOwner, npc.getCellRef().mRefID))
                    out.push_back(searchPtrViaHandle(*it));
        }
    }

    void World::enableActorCollision(const MWWorld::Ptr& actor, bool enable)
    {
        OEngine::Physic::PhysicActor *physicActor = mPhysEngine->getCharacter(actor.getRefData().getHandle());
        
        if (enable)
        {
            physicActor->enableCollisionBody();
        }
        else
        {
            physicActor->disableCollisionBody();
        }
    }

    bool World::findInteriorPosition(const std::string &name, ESM::Position &pos)
    {
        typedef MWWorld::CellRefList<ESM::Door>::List DoorList;

        pos.rot[0] = pos.rot[1] = pos.rot[2] = 0;
        pos.pos[0] = pos.pos[1] = pos.pos[2] = 0;

        MWWorld::CellStore *cellStore = getInterior(name);

        if (0 == cellStore) {
            return false;
        }
        const DoorList &doors = cellStore->mDoors.mList;
        for (DoorList::const_iterator it = doors.begin(); it != doors.end(); ++it) {
            if (!it->mRef.mTeleport) {
                continue;
            }

            MWWorld::CellStore *source = 0;

            // door to exterior
            if (it->mRef.mDestCell.empty()) {
                int x, y;
                const float *pos = it->mRef.mDoorDest.pos;
                positionToIndex(pos[0], pos[1], x, y);
                source = getExterior(x, y);
            }
            // door to interior
            else {
                source = getInterior(it->mRef.mDestCell);
            }
            if (0 != source) {
                // Find door leading to our current teleport door
                // and use it destination to position inside cell.
                const DoorList &doors = source->mDoors.mList;
                for (DoorList::const_iterator jt = doors.begin(); jt != doors.end(); ++jt) {
                    if (it->mRef.mTeleport &&
                        Misc::StringUtils::ciEqual(name, jt->mRef.mDestCell))
                    {
                        /// \note Using _any_ door pointed to the interior,
                        /// not the one pointed to current door.
                        pos = jt->mRef.mDoorDest;
                        return true;
                    }
                }
            }
        }
        return false;
    }

    bool World::findExteriorPosition(const std::string &name, ESM::Position &pos)
    {
        pos.rot[0] = pos.rot[1] = pos.rot[2] = 0;

        if (const ESM::Cell *ext = getExterior(name)) {
            int x = ext->getGridX();
            int y = ext->getGridY();
            indexToPosition(x, y, pos.pos[0], pos.pos[1], true);

            ESM::Land* land = getStore().get<ESM::Land>().search(x, y);
            if (land) {
                if (!land->isDataLoaded(ESM::Land::DATA_VHGT)) {
                    land->loadData(ESM::Land::DATA_VHGT);
                }
                pos.pos[2] = land->mLandData->mHeights[ESM::Land::LAND_NUM_VERTS / 2 + 1];
            }
            else {
                std::cerr << "Land data for cell at (" << x << ", " << y << ") not found\n";
                pos.pos[2] = 0;
            }

            return true;
        }
        return false;
    }
}
