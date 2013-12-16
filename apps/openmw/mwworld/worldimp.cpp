#include "worldimp.hpp"
#ifdef _WIN32
#include <boost/tr1/tr1/unordered_map>
#elif defined HAVE_UNORDERED_MAP
#include <unordered_map>
#else
#include <tr1/unordered_map>
#endif

#include <OgreSceneNode.h>

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
#include "../mwmechanics/npcstats.hpp"
#include "../mwmechanics/spellcasting.hpp"


#include "../mwrender/sky.hpp"
#include "../mwrender/animation.hpp"

#include "../mwclass/door.hpp"

#include "player.hpp"
#include "manualref.hpp"
#include "cellfunctors.hpp"
#include "containerstore.hpp"
#include "inventorystore.hpp"

#include "contentloader.hpp"
#include "esmloader.hpp"
#include "omwloader.hpp"

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
    struct GameContentLoader : public ContentLoader
    {
        GameContentLoader(Loading::Listener& listener)
          : ContentLoader(listener)
        {
        }

        bool addLoader(const std::string& extension, ContentLoader* loader)
        {
            return mLoaders.insert(std::make_pair(extension, loader)).second;
        }

        void load(const boost::filesystem::path& filepath, int& index)
        {
            LoadersContainer::iterator it(mLoaders.find(Misc::StringUtils::lowerCase(filepath.extension().string())));
            if (it != mLoaders.end())
            {
                it->second->load(filepath, index);
            }
            else
            {
              std::string msg("Cannot load file: ");
              msg += filepath.string();
              throw std::runtime_error(msg.c_str());
            }
        }

        private:
          typedef std::tr1::unordered_map<std::string, ContentLoader*> LoadersContainer;
          LoadersContainer mLoaders;
    };

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
        const std::vector<std::string>& contentFiles,
        const boost::filesystem::path& resDir, const boost::filesystem::path& cacheDir,
        ToUTF8::Utf8Encoder* encoder, const std::map<std::string,std::string>& fallbackMap, int mActivationDistanceOverride)
    : mPlayer (0), mLocalScripts (mStore), mGlobalVariables (0),
      mSky (true), mCells (mStore, mEsm),
      mActivationDistanceOverride (mActivationDistanceOverride),
      mFallback(fallbackMap), mPlayIntro(0), mTeleportEnabled(true), mLevitationEnabled(false),
      mFacedDistance(FLT_MAX), mGodMode(false)
    {
        mPhysics = new PhysicsSystem(renderer);
        mPhysEngine = mPhysics->getEngine();

        mRendering = new MWRender::RenderingManager(renderer, resDir, cacheDir, mPhysEngine,&mFallback);

        mPhysEngine->setSceneManager(renderer.getScene());

        mWeatherManager = new MWWorld::WeatherManager(mRendering,&mFallback);

        // NOTE: We might need to reserve one more for the running game / save.
        mEsm.resize(contentFiles.size());
        Loading::Listener* listener = MWBase::Environment::get().getWindowManager()->getLoadingScreen();
        listener->loadingOn();

        GameContentLoader gameContentLoader(*listener);
        EsmLoader esmLoader(mStore, mEsm, encoder, *listener);
        OmwLoader omwLoader(*listener);

        gameContentLoader.addLoader(".esm", &esmLoader);
        gameContentLoader.addLoader(".esp", &esmLoader);
        gameContentLoader.addLoader(".omwgame", &omwLoader);
        gameContentLoader.addLoader(".omwaddon", &omwLoader);

        loadContentFiles(fileCollections, contentFiles, gameContentLoader);

        listener->loadingOff();

        // insert records that may not be present in all versions of MW
        if (mEsm[0].getFormat() == 0)
            ensureNeededRecords();

        mStore.setUp();
        mStore.movePlayerRecord();

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
        mPlayer->setCell(NULL);
        MWWorld::Ptr player = mPlayer->getPlayer();

        // removes NpcStats, ContainerStore etc
        player.getRefData().setCustomData(NULL);

        renderPlayer();
        mRendering->resetCamera();

        // make sure to do this so that local scripts from items that were in the players inventory are removed
        mLocalScripts.clear();

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

        Ptr ptr = Class::get (mPlayer->getPlayer()).
            getContainerStore (mPlayer->getPlayer()).search (name);

        if (!ptr.isEmpty())
            return ptr;

        // active cells
        for (Scene::CellStoreCollection::const_iterator iter (mWorldScene->getActiveCells().begin());
            iter!=mWorldScene->getActiveCells().end(); ++iter)
        {
            Ptr::CellStore* cellstore = *iter;
            Ptr ptr = mCells.getPtr (name, *cellstore, true);

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
        MWBase::Environment::get().getMechanicsManager()->advanceTime(hours*3600);

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
        removeContainerScripts(getPlayer().getPlayer());
        mWorldScene->changeToInteriorCell(cellName, position);
        addContainerScripts(getPlayer().getPlayer(), getPlayer().getPlayer().getCell());
    }

    void World::changeToExteriorCell (const ESM::Position& position)
    {
        removeContainerScripts(getPlayer().getPlayer());
        mWorldScene->changeToExteriorCell(position);
        addContainerScripts(getPlayer().getPlayer(), getPlayer().getPlayer().getCell());
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
            result = mPhysics->getFacedHandle (getMaxActivationDistance ());
        else
            result = std::make_pair (mFacedDistance, mFacedHandle);

        if (result.second.empty())
            return MWWorld::Ptr ();

        MWWorld::Ptr object = searchPtrViaHandle (result.second);
        if (object.isEmpty())
            return object;
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

    std::pair<MWWorld::Ptr,Ogre::Vector3> World::getHitContact(const MWWorld::Ptr &ptr, float distance)
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

        std::pair<std::string,Ogre::Vector3> result = mPhysics->getHitContact(ptr.getRefData().getHandle(),
                                                                              pos, rot, distance);
        if(result.first.empty())
            return std::make_pair(MWWorld::Ptr(), Ogre::Vector3(0.0f));

        return std::make_pair(searchPtrViaHandle(result.first), result.second);
    }

    void World::deleteObject (const Ptr& ptr)
    {
        if (ptr.getRefData().getCount() > 0)
        {
            ptr.getRefData().setCount(0);

            if (ptr.isInCell()
                && mWorldScene->getActiveCells().find(ptr.getCell()) != mWorldScene->getActiveCells().end()
                && ptr.getRefData().isEnabled())
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
                addContainerScripts (getPlayer().getPlayer(), &newCell);
            }
            else
            {
                if (!mWorldScene->isCellActive(*currCell))
                    copyObjectToCell(ptr, newCell, pos);
                else if (!mWorldScene->isCellActive(newCell))
                {
                    mWorldScene->removeObjectFromScene(ptr);
                    mLocalScripts.remove(ptr);
                    removeContainerScripts (ptr);
                    haveToMove = false;

                    MWWorld::Ptr newPtr = MWWorld::Class::get(ptr)
                            .copyToCell(ptr, newCell);
                    newPtr.getRefData().setBaseNode(0);

                    objectLeftActiveCell(ptr, newPtr);
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

    MWWorld::Ptr World::safePlaceObject(const MWWorld::Ptr& ptr,MWWorld::CellStore &Cell,ESM::Position pos)
    {
        return copyObjectToCell(ptr,Cell,pos);
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

        cellX = std::floor(x/cellSize);
        cellY = std::floor(y/cellSize);
    }

    void World::queueMovement(const Ptr &ptr, const Vector3 &velocity)
    {
        mPhysics->queueObjectMovement(ptr, velocity);
    }

    void World::doPhysics(float duration)
    {
        processDoors(duration);

        moveProjectiles(duration);

        const PtrVelocityList &results = mPhysics->applyQueuedMovement(duration);
        PtrVelocityList::const_iterator player(results.end());
        for(PtrVelocityList::const_iterator iter(results.begin());iter != results.end();iter++)
        {
            if(iter->first.getRefData().getHandle() == "player")
            {
                /* Handle player last, in case a cell transition occurs */
                player = iter;
                continue;
            }
            moveObjectImp(iter->first, iter->second.x, iter->second.y, iter->second.z);
        }
        if(player != results.end())
            moveObjectImp(player->first, player->second.x, player->second.y, player->second.z);

        mPhysEngine->stepSimulation(duration);
    }

    bool World::castRay (float x1, float y1, float z1, float x2, float y2, float z2)
    {
        Ogre::Vector3 a(x1,y1,z1);
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
        return mPhysics->toggleCollisionMode();
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

        if (!paused)
            doPhysics (duration);

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

        if (results.empty())
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

    void World::modRegion(const std::string &regionid, const std::vector<char> &chances)
    {
        mWeatherManager->modRegion(regionid, chances);
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
            item.getRefData().getLocals().setVarByInt(script, "onpcdrop", 1);
    }

    bool World::placeObject (const MWWorld::Ptr& object, float cursorX, float cursorY, int amount)
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

        // copy the object and set its count
        int origCount = object.getRefData().getCount();
        object.getRefData().setCount(amount);
        Ptr dropped = copyObjectToCell(object, *cell, pos);
        object.getRefData().setCount(origCount);

        // only the player place items in the world, so no need to check actor
        PCDropped(dropped);

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

        if (object.getClass().isActor())
        {
            Ogre::Vector3 min, max;
            if (mPhysics->getObjectAABB(object, min, max)) {
                float *pos = dropped.getRefData().getPosition().pos;
                pos[0] -= (min.x + max.x) / 2;
                pos[1] -= (min.y + max.y) / 2;
                pos[2] -= min.z;
            }
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

    void World::dropObjectOnGround (const Ptr& actor, const Ptr& object, int amount)
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

        // copy the object and set its count
        int origCount = object.getRefData().getCount();
        object.getRefData().setCount(amount);
        Ptr dropped = copyObjectToCell(object, *cell, pos);
        object.getRefData().setCount(origCount);

        if(actor == mPlayer->getPlayer()) // Only call if dropped by player
            PCDropped(dropped);
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
        if(!ptr.getClass().isActor())
            return false;

        const MWMechanics::CreatureStats &stats = ptr.getClass().getCreatureStats(ptr);
        if(stats.getMagicEffects().get(MWMechanics::EffectKey(ESM::MagicEffect::Levitate)).mMagnitude > 0)
            return true;

        // TODO: Check if flying creature

        const OEngine::Physic::PhysicActor *actor = mPhysEngine->getCharacter(ptr.getRefData().getHandle());
        if(!actor || !actor->getCollisionMode())
            return true;

        return false;
    }

    bool
    World::isSlowFalling(const MWWorld::Ptr &ptr) const
    {
        if(!ptr.getClass().isActor())
            return false;

        const MWMechanics::CreatureStats &stats = ptr.getClass().getCreatureStats(ptr);
        if(stats.getMagicEffects().get(MWMechanics::EffectKey(ESM::MagicEffect::SlowFall)).mMagnitude > 0)
            return true;

        return false;
    }

    bool World::isSubmerged(const MWWorld::Ptr &object) const
    {
        float *fpos = object.getRefData().getPosition().pos;
        Ogre::Vector3 pos(fpos[0], fpos[1], fpos[2]);

        const OEngine::Physic::PhysicActor *actor = mPhysEngine->getCharacter(object.getRefData().getHandle());
        if(actor) pos.z += 1.85*actor->getHalfExtents().z;

        return isUnderwater(object.getCell(), pos);
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

        Ptr player = mPlayer->getPlayer();
        RefData &refdata = player.getRefData();
        Ogre::Vector3 playerPos(refdata.getPosition().pos);

        const OEngine::Physic::PhysicActor *physactor = mPhysEngine->getCharacter(refdata.getHandle());
        if((!physactor->getOnGround()&&physactor->getCollisionMode()) || isUnderwater(currentCell, playerPos))
            return 2;
        if((currentCell->mCell->mData.mFlags&ESM::Cell::NoSleep) ||
           Class::get(player).getNpcStats(player).isWerewolf())
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

    void World::frameStarted (float dt, bool paused)
    {
        mRendering->frameStarted(dt, paused);
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
                if (Misc::StringUtils::ciEqual(searchPtrViaHandle(*it).getCellRef().mOwner, npc.getCellRef().mRefID))
                    out.push_back(searchPtrViaHandle(*it));
        }
    }

    bool World::getLOS(const MWWorld::Ptr& npc,const MWWorld::Ptr& targetNpc)
    {
        // This is a placeholder! Needs to go into an NPC awareness check function (see
        // https://wiki.openmw.org/index.php?title=Research:NPC_AI_Behaviour#NPC_Awareness_Check )
        if (targetNpc.getClass().getCreatureStats(targetNpc).getMagicEffects().get(ESM::MagicEffect::Invisibility).mMagnitude)
            return false;
        if (targetNpc.getClass().getCreatureStats(targetNpc).getMagicEffects().get(ESM::MagicEffect::Chameleon).mMagnitude > 100)
            return false;

        Ogre::Vector3 halfExt1 = mPhysEngine->getCharacter(npc.getRefData().getHandle())->getHalfExtents();
        float* pos1 = npc.getRefData().getPosition().pos;
        Ogre::Vector3 halfExt2 = mPhysEngine->getCharacter(targetNpc.getRefData().getHandle())->getHalfExtents();
        float* pos2 = targetNpc.getRefData().getPosition().pos;

        btVector3 from(pos1[0],pos1[1],pos1[2]+halfExt1.z);
        btVector3 to(pos2[0],pos2[1],pos2[2]+halfExt2.z);

        std::pair<std::string, float> result = mPhysEngine->rayTest(from, to,false);
        if(result.first == "") return true;
        return false;
    }

    void World::enableActorCollision(const MWWorld::Ptr& actor, bool enable)
    {
        OEngine::Physic::PhysicActor *physicActor = mPhysEngine->getCharacter(actor.getRefData().getHandle());

        physicActor->enableCollisions(enable);
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

    void World::enableTeleporting(bool enable)
    {
        mTeleportEnabled = enable;
    }

    bool World::isTeleportingEnabled() const
    {
        return mTeleportEnabled;
    }

    void World::enableLevitation(bool enable)
    {
        mLevitationEnabled = enable;
    }

    bool World::isLevitationEnabled() const
    {
        return mLevitationEnabled;
    }

    void World::setWerewolf(const MWWorld::Ptr& actor, bool werewolf)
    {
        MWMechanics::NpcStats& npcStats = Class::get(actor).getNpcStats(actor);

        // The actor does not have to change state
        if (npcStats.isWerewolf() == werewolf)
            return;

        npcStats.setWerewolf(werewolf);

        MWWorld::InventoryStore& invStore = MWWorld::Class::get(actor).getInventoryStore(actor);
        invStore.unequipAll(actor);

        if(werewolf)
        {
            InventoryStore &inv = actor.getClass().getInventoryStore(actor);

            inv.equip(InventoryStore::Slot_Robe, inv.ContainerStore::add("WerewolfRobe", 1, actor), actor);
        }
        else
        {
            actor.getClass().getContainerStore(actor).remove("WerewolfRobe", 1, actor);
        }

        if(actor.getRefData().getHandle() == "player")
        {
            // Update the GUI only when called on the player
            MWBase::WindowManager* windowManager = MWBase::Environment::get().getWindowManager();
            windowManager->unsetSelectedWeapon();

            if (werewolf)
            {
                windowManager->forceHide(MWGui::GW_Inventory);
                windowManager->forceHide(MWGui::GW_Magic);
            }
            else
            {
                windowManager->unsetForceHide(MWGui::GW_Inventory);
                windowManager->unsetForceHide(MWGui::GW_Magic);
            }
        }

        mRendering->rebuildPtr(actor);
    }

    void World::applyWerewolfAcrobatics(const Ptr &actor)
    {
        const Store<ESM::GameSetting> &gmst = getStore().get<ESM::GameSetting>();
        MWMechanics::NpcStats &stats = Class::get(actor).getNpcStats(actor);

        stats.getSkill(ESM::Skill::Acrobatics).setModified(gmst.find("fWerewolfAcrobatics")->getFloat(), 0);
    }

    bool World::getGodModeState()
    {
        return mGodMode;
    }

    bool World::toggleGodMode()
    {
        mGodMode = !mGodMode;

        return mGodMode;
    }

    void World::loadContentFiles(const Files::Collections& fileCollections,
        const std::vector<std::string>& content, ContentLoader& contentLoader)
    {
        std::vector<std::string>::const_iterator it(content.begin());
        std::vector<std::string>::const_iterator end(content.end());
        for (int idx = 0; it != end; ++it, ++idx)
        {
            boost::filesystem::path filename(*it);
            const Files::MultiDirCollection& col = fileCollections.getCollection(filename.extension().string());
            if (col.doesExist(*it))
            {
                contentLoader.load(col.getPath(*it), idx);
            }
        }
    }

    void World::castSpell(const Ptr &actor)
    {
        MWMechanics::CreatureStats& stats = actor.getClass().getCreatureStats(actor);
        InventoryStore& inv = actor.getClass().getInventoryStore(actor);

        // Unset casting flag, otherwise pressing the mouse button down would continue casting every frame if using an enchantment
        // (which casts instantly without an animation)
        stats.setAttackingOrSpell(false);

        MWWorld::Ptr target = getFacedObject();

        std::string selectedSpell = stats.getSpells().getSelectedSpell();

        MWMechanics::CastSpell cast(actor, target);

        if (!selectedSpell.empty())
        {
            const ESM::Spell* spell = getStore().get<ESM::Spell>().search(selectedSpell);

            cast.cast(spell);
        }
        else if (inv.getSelectedEnchantItem() != inv.end())
        {
            cast.cast(*inv.getSelectedEnchantItem());
        }
    }

    void World::launchProjectile (const std::string& id, bool stack, const ESM::EffectList& effects,
                                   const MWWorld::Ptr& actor, const std::string& sourceName)
    {
        std::string projectileModel;
        std::string sound;
        float speed = 0;
        for (std::vector<ESM::ENAMstruct>::const_iterator iter (effects.mList.begin());
            iter!=effects.mList.end(); ++iter)
        {
            if (iter->mRange != ESM::RT_Target)
                continue;

            const ESM::MagicEffect *magicEffect = getStore().get<ESM::MagicEffect>().find (
                iter->mEffectID);

            projectileModel = magicEffect->mBolt;
            if (projectileModel.empty())
                projectileModel = "VFX_DefaultBolt";

            static const std::string schools[] = {
                "alteration", "conjuration", "destruction", "illusion", "mysticism", "restoration"
            };

            if (!magicEffect->mBoltSound.empty())
                sound = magicEffect->mBoltSound;
            else
                sound = schools[magicEffect->mData.mSchool] + " bolt";

            speed = magicEffect->mData.mSpeed;
            break;
        }
        if (projectileModel.empty())
            return;

        // Spawn at 0.75 * ActorHeight
        float height = mPhysEngine->getCharacter(actor.getRefData().getHandle())->getHalfExtents().z * 2 * 0.75;

        MWWorld::ManualRef ref(getStore(), projectileModel);
        ESM::Position pos;
        pos.pos[0] = actor.getRefData().getPosition().pos[0];
        pos.pos[1] = actor.getRefData().getPosition().pos[1];
        pos.pos[2] = actor.getRefData().getPosition().pos[2] + height;
        pos.rot[0] = actor.getRefData().getPosition().rot[0];
        pos.rot[1] = actor.getRefData().getPosition().rot[1];
        pos.rot[2] = actor.getRefData().getPosition().rot[2];
        ref.getPtr().getCellRef().mPos = pos;
        MWWorld::Ptr ptr = copyObjectToCell(ref.getPtr(), *actor.getCell(), pos);

        ProjectileState state;
        state.mSourceName = sourceName;
        state.mId = id;
        state.mActorHandle = actor.getRefData().getHandle();
        state.mSpeed = speed;
        state.mEffects = effects;
        state.mStack = stack;

        MWBase::SoundManager *sndMgr = MWBase::Environment::get().getSoundManager();
        sndMgr->playSound3D(ptr, sound, 1.0f, 1.0f);

        mProjectiles[ptr] = state;
    }

    void World::moveProjectiles(float duration)
    {
        std::map<std::string, ProjectileState> moved;
        for (std::map<MWWorld::Ptr, ProjectileState>::iterator it = mProjectiles.begin(); it != mProjectiles.end();)
        {
            if (!mWorldScene->isCellActive(*it->first.getCell()))
            {
                mProjectiles.erase(it++);
                continue;
            }

            MWWorld::Ptr ptr = it->first;

            Ogre::Vector3 rot(ptr.getRefData().getPosition().rot);

            // TODO: Why -rot.z, but not -rot.x?
            Ogre::Quaternion orient = Ogre::Quaternion(Ogre::Radian(-rot.z), Ogre::Vector3::UNIT_Z);
            orient = orient * Ogre::Quaternion(Ogre::Radian(rot.x), Ogre::Vector3::UNIT_X);

            // This is just a guess, probably wrong
            static float fProjectileMinSpeed = getStore().get<ESM::GameSetting>().find("fProjectileMinSpeed")->getFloat();
            static float fProjectileMaxSpeed = getStore().get<ESM::GameSetting>().find("fProjectileMaxSpeed")->getFloat();
            float speed = fProjectileMinSpeed + (fProjectileMaxSpeed - fProjectileMinSpeed) * it->second.mSpeed;

            Ogre::Vector3 direction = orient.yAxis();
            direction.normalise();
            Ogre::Vector3 pos(ptr.getRefData().getPosition().pos);
            Ogre::Vector3 newPos = pos + direction * duration * speed;

            // Check for impact
            btVector3 from(pos.x, pos.y, pos.z);
            btVector3 to(newPos.x, newPos.y, newPos.z);
            std::vector<std::pair<float, std::string> > collisions = mPhysEngine->rayTest2(from, to);
            bool explode = false;
            for (std::vector<std::pair<float, std::string> >::iterator cIt = collisions.begin(); cIt != collisions.end() && !explode; ++cIt)
            {
                MWWorld::Ptr obstacle = searchPtrViaHandle(cIt->second);
                if (obstacle == ptr)
                    continue;

                explode = true;

                MWWorld::Ptr caster = searchPtrViaHandle(it->second.mActorHandle);
                if (caster.isEmpty())
                    caster = obstacle;
                if (obstacle.isEmpty())
                {
                    // Terrain
                }
                else
                {
                    MWMechanics::CastSpell cast(caster, obstacle);
                    cast.mStack = it->second.mStack;
                    cast.mId = it->second.mId;
                    cast.mSourceName = it->second.mSourceName;
                    cast.inflict(obstacle, caster, it->second.mEffects, ESM::RT_Target, false);
                }

                deleteObject(ptr);
                mProjectiles.erase(it++);
            }

            if (explode)
            {
                // TODO: Explode
                continue;
            }

            std::string handle = ptr.getRefData().getHandle();

            moveObject(ptr, newPos.x, newPos.y, newPos.z);

            // HACK: Re-fetch Ptrs if necessary, since the cell might have changed
            if (!ptr.getRefData().getCount())
            {
                moved[handle] = it->second;
                mProjectiles.erase(it++);
            }
            else
                ++it;
        }

        // HACK: Re-fetch Ptrs if necessary, since the cell might have changed
        for (std::map<std::string, ProjectileState>::iterator it = moved.begin(); it != moved.end(); ++it)
        {
            MWWorld::Ptr newPtr = searchPtrViaHandle(it->first);
            if (newPtr.isEmpty()) // The projectile went into an inactive cell and was deleted
                continue;
            mProjectiles[getPtrViaHandle(it->first)] = it->second;
        }
    }

    void World::objectLeftActiveCell(Ptr object, Ptr movedPtr)
    {
        // For now, projectiles moved to an inactive cell are just deleted, because there's no reliable way to hold on to the meta information
        if (mProjectiles.find(object) != mProjectiles.end())
        {
            deleteObject(movedPtr);
        }
    }

    void World::breakInvisibility(const Ptr &actor)
    {
        actor.getClass().getCreatureStats(actor).getActiveSpells().purgeEffect(ESM::MagicEffect::Invisibility);
        actor.getClass().getInventoryStore(actor).purgeEffect(ESM::MagicEffect::Invisibility);
    }
}
