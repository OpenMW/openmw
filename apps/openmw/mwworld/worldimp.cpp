#include "worldimp.hpp"

#ifdef _WIN32
#include <boost/tr1/tr1/unordered_map>
#elif defined HAVE_UNORDERED_MAP
#include <unordered_map>
#else
#include <tr1/unordered_map>
#endif
#include "../mwbase/scriptmanager.hpp"
#include "../mwscript/globalscripts.hpp"
#include <OgreSceneNode.h>

#include <libs/openengine/bullet/trace.h>
#include <libs/openengine/bullet/physic.hpp>

#include <components/bsa/bsa_archive.hpp>
#include <components/files/collections.hpp>
#include <components/compiler/locals.hpp>
#include <components/esm/cellid.hpp>

#include <boost/math/special_functions/sign.hpp>

#include "../mwbase/environment.hpp"
#include "../mwbase/soundmanager.hpp"
#include "../mwbase/mechanicsmanager.hpp"
#include "../mwbase/windowmanager.hpp"

#include "../mwmechanics/creaturestats.hpp"
#include "../mwmechanics/movement.hpp"
#include "../mwmechanics/npcstats.hpp"
#include "../mwmechanics/spellcasting.hpp"
#include "../mwmechanics/levelledlist.hpp"
#include "../mwmechanics/combat.hpp"
#include "../mwmechanics/aiavoiddoor.hpp" //Used to tell actors to avoid doors

#include "../mwrender/sky.hpp"
#include "../mwrender/animation.hpp"

#include "../mwscript/interpretercontext.hpp"

#include "../mwclass/door.hpp"

#include "player.hpp"
#include "manualref.hpp"
#include "cellfunctors.hpp"
#include "containerstore.hpp"
#include "inventorystore.hpp"
#include "actionteleport.hpp"
#include "projectilemanager.hpp"

#include "contentloader.hpp"
#include "esmloader.hpp"
#include "omwloader.hpp"

using namespace Ogre;

namespace
{

// Wraps a value to (-PI, PI]
void wrap(float& rad)
{
    if (rad>0)
        rad = std::fmod(rad+Ogre::Math::PI, 2.0f*Ogre::Math::PI)-Ogre::Math::PI;
    else
        rad = std::fmod(rad-Ogre::Math::PI, 2.0f*Ogre::Math::PI)+Ogre::Math::PI;
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
            mRendering->skySetHour (mGlobalVariables["gamehour"].getFloat());
            mRendering->skySetDate (mGlobalVariables["day"].getInteger(),
                mGlobalVariables["month"].getInteger());

            mRendering->skyEnable();
        }
        else
            mRendering->skyDisable();
    }

    World::World (OEngine::Render::OgreRenderer& renderer,
        const Files::Collections& fileCollections,
        const std::vector<std::string>& contentFiles,
        const boost::filesystem::path& resDir, const boost::filesystem::path& cacheDir,
        ToUTF8::Utf8Encoder* encoder, const std::map<std::string,std::string>& fallbackMap,
        int activationDistanceOverride, const std::string& startCell, const std::string& startupScript)
    : mPlayer (0), mLocalScripts (mStore),
      mSky (true), mCells (mStore, mEsm),
      mActivationDistanceOverride (activationDistanceOverride),
      mFallback(fallbackMap), mTeleportEnabled(true), mLevitationEnabled(true),
      mGodMode(false), mContentFiles (contentFiles),
      mGoToJail(false),
      mStartCell (startCell), mStartupScript(startupScript)
    {
        mPhysics = new PhysicsSystem(renderer);
        mPhysEngine = mPhysics->getEngine();

        mProjectileManager.reset(new ProjectileManager(renderer.getScene(), *mPhysEngine));

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

        mGlobalVariables.fill (mStore);

        mWorldScene = new Scene(*mRendering, mPhysics);
    }

    void World::startNewGame (bool bypass)
    {
        mGoToJail = false;
        mLevitationEnabled = true;
        mTeleportEnabled = true;

        // Rebuild player
        setupPlayer();

        renderPlayer();

        MWBase::Environment::get().getWindowManager()->updatePlayer();

        if (!bypass)
        {
            // set new game mark
            mGlobalVariables["chargenstate"].setInteger (1);
            mGlobalVariables["pcrace"].setInteger (3);
        }
        else
            mGlobalVariables["chargenstate"].setInteger (-1);

        if (bypass && !mStartCell.empty())
        {
            ESM::Position pos;

            if (findExteriorPosition (mStartCell, pos))
            {
                changeToExteriorCell (pos);
            }
            else
            {
                findInteriorPosition (mStartCell, pos);
                changeToInteriorCell (mStartCell, pos);
            }
        }
        else
        {
            for (int i=0; i<5; ++i)
                MWBase::Environment::get().getScriptManager()->getGlobalScripts().run();
            if (!getPlayerPtr().isInCell())
            {
                ESM::Position pos;
                const int cellSize = 8192;
                pos.pos[0] = cellSize/2;
                pos.pos[1] = cellSize/2;
                pos.pos[2] = 0;
                pos.rot[0] = 0;
                pos.rot[1] = 0;
                pos.rot[2] = 0;
                mWorldScene->changeToExteriorCell(pos);
            }
        }

        if (!bypass)
            MWBase::Environment::get().getWindowManager()->playVideo(mFallback.getFallbackString("Movies_New_Game"), true);

        // enable collision
        if (!mPhysics->toggleCollisionMode())
            mPhysics->toggleCollisionMode();

        // we don't want old weather to persist on a new game
        delete mWeatherManager;
        mWeatherManager = 0;
        mWeatherManager = new MWWorld::WeatherManager(mRendering,&mFallback);

        MWBase::Environment::get().getWindowManager()->executeInConsole(mStartupScript);
    }

    void World::clear()
    {
        mRendering->clear();

        mProjectileManager->clear();

        mLocalScripts.clear();

        mWorldScene->changeToVoid();

        mStore.clearDynamic();
        mStore.setUp();

        if (mPlayer)
        {
	    mPlayer->clear();
            mPlayer->setCell(0);
            mPlayer->getPlayer().getRefData() = RefData();
            mPlayer->set(mStore.get<ESM::NPC>().find ("player"));
        }

        mCells.clear();

        mDoorStates.clear();

        mGodMode = false;
        mSky = true;
        mTeleportEnabled = true;

        mGlobalVariables.fill (mStore);
    }

    int World::countSavedGameRecords() const
    {
        return
            mCells.countSavedGameRecords()
            +mStore.countSavedGameRecords()
            +mGlobalVariables.countSavedGameRecords()
            +mProjectileManager->countSavedGameRecords()
            +1 // player record
            +1 // weather record
            +1; // actorId counter
    }

    void World::write (ESM::ESMWriter& writer, Loading::Listener& progress) const
    {
        // Active cells could have a dirty fog of war, sync it to the CellStore first
        for (Scene::CellStoreCollection::const_iterator iter (mWorldScene->getActiveCells().begin());
            iter!=mWorldScene->getActiveCells().end(); ++iter)
        {
            CellStore* cellstore = *iter;
            mRendering->writeFog(cellstore);
        }

        MWMechanics::CreatureStats::writeActorIdCounter(writer);
        progress.increaseProgress();

        mStore.write (writer, progress); // dynamic Store must be written (and read) before Cells, so that
                                         // references to custom made records will be recognized
        mCells.write (writer, progress);
        mGlobalVariables.write (writer, progress);
        mPlayer->write (writer, progress);
        mWeatherManager->write (writer, progress);
        mProjectileManager->write (writer, progress);
    }

    void World::readRecord (ESM::ESMReader& reader, int32_t type,
        const std::map<int, int>& contentFileMap)
    {
        if (type == ESM::REC_ACTC)
        {
            MWMechanics::CreatureStats::readActorIdCounter(reader);
            return;
        }

        if (!mStore.readRecord (reader, type) &&
            !mGlobalVariables.readRecord (reader, type) &&
            !mPlayer->readRecord (reader, type) &&
            !mWeatherManager->readRecord (reader, type) &&
            !mCells.readRecord (reader, type, contentFileMap) &&
            !mProjectileManager->readRecord (reader, type))
        {
            throw std::runtime_error ("unknown record in saved game");
        }
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
        // Must be cleared before mRendering is destroyed
        mProjectileManager->clear();

        delete mWeatherManager;
        delete mWorldScene;
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

    CellStore *World::getExterior (int x, int y)
    {
        return mCells.getExterior (x, y);
    }

    CellStore *World::getInterior (const std::string& name)
    {
        return mCells.getInterior (name);
    }

    CellStore *World::getCell (const ESM::CellId& id)
    {
        if (id.mPaged)
            return getExterior (id.mIndex.mX, id.mIndex.mY);
        else
            return getInterior (id.mWorldspace);
    }

    void World::useDeathCamera()
    {
        if(mRendering->getCamera()->isVanityOrPreviewModeEnabled() )
        {
            mRendering->getCamera()->togglePreviewMode(false);
            mRendering->getCamera()->toggleVanityMode(false);
        }
        if(mRendering->getCamera()->isFirstPerson())
            mRendering->getCamera()->toggleViewMode(true);
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

    void World::setGlobalInt (const std::string& name, int value)
    {
        if (name=="gamehour")
            setHour (value);
        else if (name=="day")
            setDay (value);
        else if (name=="month")
            setMonth (value);
        else
            mGlobalVariables[name].setInteger (value);
    }

    void World::setGlobalFloat (const std::string& name, float value)
    {
        if (name=="gamehour")
            setHour (value);
        else if (name=="day")
            setDay (value);
        else if (name=="month")
            setMonth (value);
        else
            mGlobalVariables[name].setFloat (value);
    }

    int World::getGlobalInt (const std::string& name) const
    {
        return mGlobalVariables[name].getInteger();
    }

    float World::getGlobalFloat (const std::string& name) const
    {
        return mGlobalVariables[name].getFloat();
    }

    char World::getGlobalVariableType (const std::string& name) const
    {
        return mGlobalVariables.getType (name);
    }

    std::string World::getCellName (const MWWorld::CellStore *cell) const
    {
        if (!cell)
            cell = mWorldScene->getCurrentCell();

        if (!cell->getCell()->isExterior() || !cell->getCell()->mName.empty())
            return cell->getCell()->mName;

        if (const ESM::Region* region = getStore().get<ESM::Region>().search (cell->getCell()->mRegion))
            return region->mName;

        return getStore().get<ESM::GameSetting>().find ("sDefaultCellname")->mValue.getString();
    }

    void World::removeRefScript (MWWorld::RefData *ref)
    {
        mLocalScripts.remove (ref);
    }

    Ptr World::searchPtr (const std::string& name, bool activeOnly)
    {
        Ptr ret;
        // the player is always in an active cell.
        if (name=="player")
        {
            return mPlayer->getPlayer();
        }

        std::string lowerCaseName = Misc::StringUtils::lowerCase(name);

        // active cells
        for (Scene::CellStoreCollection::const_iterator iter (mWorldScene->getActiveCells().begin());
            iter!=mWorldScene->getActiveCells().end(); ++iter)
        {
            CellStore* cellstore = *iter;
            Ptr ptr = mCells.getPtr (lowerCaseName, *cellstore, true);

            if (!ptr.isEmpty())
                return ptr;
        }

        Ptr ptr = mPlayer->getPlayer().getClass()
            .getContainerStore(mPlayer->getPlayer()).search(lowerCaseName);

        if (!ptr.isEmpty())
            return ptr;

        if (!activeOnly)
        {
            ret = mCells.getPtr (lowerCaseName);
        }
        return ret;
    }

    Ptr World::getPtr (const std::string& name, bool activeOnly)
    {
        Ptr ret = searchPtr(name, activeOnly);
        if (!ret.isEmpty())
            return ret;
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

        return mWorldScene->searchPtrViaHandle (handle);
    }

    Ptr World::searchPtrViaActorId (int actorId)
    {
        // The player is not registered in any CellStore so must be checked manually
        if (actorId == getPlayerPtr().getClass().getCreatureStats(getPlayerPtr()).getActorId())
            return getPlayerPtr();
        // Now search cells
        return mWorldScene->searchPtrViaActorId (actorId);
    }

    void World::addContainerScripts(const Ptr& reference, CellStore * cell)
    {
        if( reference.getTypeName()==typeid (ESM::Container).name() ||
            reference.getTypeName()==typeid (ESM::NPC).name() ||
            reference.getTypeName()==typeid (ESM::Creature).name())
        {
            MWWorld::ContainerStore& container = reference.getClass().getContainerStore(reference);
            for(MWWorld::ContainerStoreIterator it = container.begin(); it != container.end(); ++it)
            {
                std::string script = it->getClass().getScript(*it);
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
        // enable is a no-op for items in containers
        if (!reference.isInCell())
            return;

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
            MWWorld::ContainerStore& container = reference.getClass().getContainerStore(reference);
            for(MWWorld::ContainerStoreIterator it = container.begin(); it != container.end(); ++it)
            {
                std::string script = it->getClass().getScript(*it);
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
        // disable is a no-op for items in containers
        if (!reference.isInCell())
            return;

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

        hours += mGlobalVariables["gamehour"].getFloat();

        setHour (hours);

        int days = hours / 24;

        if (days>0)
            mGlobalVariables["dayspassed"].setInteger (
                days + mGlobalVariables["dayspassed"].getInteger());
    }

    void World::setHour (double hour)
    {
        if (hour<0)
            hour = 0;

        int days = hour / 24;

        hour = std::fmod (hour, 24);

        mGlobalVariables["gamehour"].setFloat (hour);

        mRendering->skySetHour (hour);

        mWeatherManager->setHour (hour);

        if (days>0)
            setDay (days + mGlobalVariables["day"].getInteger());
    }

    void World::setDay (int day)
    {
        if (day<1)
            day = 1;

        int month = mGlobalVariables["month"].getInteger();

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
                mGlobalVariables["year"].setInteger (mGlobalVariables["year"].getInteger()+1);
            }

            day -= days;
        }

        mGlobalVariables["day"].setInteger (day);
        mGlobalVariables["month"].setInteger (month);

        mRendering->skySetDate (day, month);
    }

    void World::setMonth (int month)
    {
        if (month<0)
            month = 0;

        int years = month / 12;
        month = month % 12;

        int days = getDaysPerMonth (month);

        if (mGlobalVariables["day"].getInteger()>days)
            mGlobalVariables["day"].setInteger (days);

        mGlobalVariables["month"].setInteger (month);

        if (years>0)
            mGlobalVariables["year"].setInteger (years+mGlobalVariables["year"].getInteger());

        mRendering->skySetDate (mGlobalVariables["day"].getInteger(), month);
    }

    int World::getDay() const
    {
        return mGlobalVariables["day"].getInteger();
    }

    int World::getMonth() const
    {
        return mGlobalVariables["month"].getInteger();
    }

    int World::getYear() const
    {
        return mGlobalVariables["year"].getInteger();
    }

    std::string World::getMonthName (int month) const
    {
        if (month==-1)
            month = getMonth();

        const int months = 12;

        if (month<0 || month>=months)
            return "";

        static const char *monthNames[months] =
        {
            "sMonthMorningstar", "sMonthSunsdawn", "sMonthFirstseed", "sMonthRainshand",
            "sMonthSecondseed", "sMonthMidyear", "sMonthSunsheight", "sMonthLastseed",
            "sMonthHeartfire", "sMonthFrostfall", "sMonthSunsdusk", "sMonthEveningstar"
        };

        return getStore().get<ESM::GameSetting>().find (monthNames[month])->mValue.getString();
    }

    TimeStamp World::getTimeStamp() const
    {
        return TimeStamp (mGlobalVariables["gamehour"].getFloat(),
            mGlobalVariables["dayspassed"].getInteger());
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
        return mGlobalVariables["timescale"].getFloat();
    }

    void World::changeToInteriorCell (const std::string& cellName, const ESM::Position& position)
    {
        if (mCurrentWorldSpace != cellName)
        {
            // changed worldspace
            mProjectileManager->clear();
            mRendering->notifyWorldSpaceChanged();
            mCurrentWorldSpace = cellName;
        }

        removeContainerScripts(getPlayerPtr());
        mWorldScene->changeToInteriorCell(cellName, position);
        addContainerScripts(getPlayerPtr(), getPlayerPtr().getCell());
    }

    void World::changeToExteriorCell (const ESM::Position& position)
    {
        if (mCurrentWorldSpace != "sys::default") // FIXME
        {
            // changed worldspace
            mProjectileManager->clear();
            mRendering->notifyWorldSpaceChanged();
        }
        removeContainerScripts(getPlayerPtr());
        mWorldScene->changeToExteriorCell(position);
        addContainerScripts(getPlayerPtr(), getPlayerPtr().getCell());
    }

    void World::changeToCell (const ESM::CellId& cellId, const ESM::Position& position, bool detectWorldSpaceChange)
    {
        if (!detectWorldSpaceChange)
            mCurrentWorldSpace = cellId.mWorldspace;

        if (cellId.mPaged)
            changeToExteriorCell (position);
        else
            changeToInteriorCell (cellId.mWorldspace, position);
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
        if (mFacedHandle.empty())
            return MWWorld::Ptr();
        return searchPtrViaHandle(mFacedHandle);
    }

    std::pair<MWWorld::Ptr,Ogre::Vector3> World::getHitContact(const MWWorld::Ptr &ptr, float distance)
    {
        const ESM::Position &posdata = ptr.getRefData().getPosition();
        Ogre::Vector3 pos(posdata.pos);
        Ogre::Quaternion rot = Ogre::Quaternion(Ogre::Radian(posdata.rot[2]), Ogre::Vector3::NEGATIVE_UNIT_Z) *
                               Ogre::Quaternion(Ogre::Radian(posdata.rot[0]), Ogre::Vector3::NEGATIVE_UNIT_X);

        MWRender::Animation *anim = mRendering->getAnimation(ptr);
        if(anim != NULL)
        {
            Ogre::Node *node = anim->getNode("Head");
            if (node == NULL)
                node = anim->getNode("Bip01 Head");
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

    void World::moveObject(const Ptr &ptr, CellStore* newCell, float x, float y, float z)
    {
        ESM::Position pos = ptr.getRefData().getPosition();

        pos.pos[0] = x;
        pos.pos[1] = y;
        pos.pos[2] = z;

        ptr.getRefData().setPosition(pos);

        Ogre::Vector3 vec(x, y, z);

        CellStore *currCell = ptr.isInCell() ? ptr.getCell() : NULL; // currCell == NULL should only happen for player, during initial startup
        bool isPlayer = ptr == mPlayer->getPlayer();
        bool haveToMove = isPlayer || mWorldScene->isCellActive(*currCell);

        if (currCell != newCell)
        {
            removeContainerScripts(ptr);

            if (isPlayer)
            {
                if (!newCell->isExterior())
                    changeToInteriorCell(Misc::StringUtils::lowerCase(newCell->getCell()->mName), pos);
                else
                {
                    int cellX = newCell->getCell()->getGridX();
                    int cellY = newCell->getCell()->getGridY();
                    mWorldScene->changeCell(cellX, cellY, pos, false);
                }
                addContainerScripts (getPlayerPtr(), newCell);
            }
            else
            {
                bool currCellActive = mWorldScene->isCellActive(*currCell);
                bool newCellActive = mWorldScene->isCellActive(*newCell);
                if (!currCellActive && newCellActive)
                {
                    MWWorld::Ptr newPtr = ptr.getClass().copyToCell(ptr, *newCell, pos);
                    mWorldScene->addObjectToScene(newPtr);

                    std::string script = newPtr.getClass().getScript(newPtr);
                    if (!script.empty()) {
                        mLocalScripts.add(script, newPtr);
                    }
                    addContainerScripts(newPtr, newCell);
                }
                else if (!newCellActive && currCellActive)
                {
                    mWorldScene->removeObjectFromScene(ptr);
                    mLocalScripts.remove(ptr);
                    removeContainerScripts (ptr);
                    haveToMove = false;

                    MWWorld::Ptr newPtr = ptr.getClass()
                            .copyToCell(ptr, *newCell);
                    newPtr.getRefData().setBaseNode(0);
                }
                else if (!currCellActive && !newCellActive)
                    ptr.getClass().copyToCell(ptr, *newCell);
                else // both cells active
                {
                    MWWorld::Ptr copy =
                        ptr.getClass().copyToCell(ptr, *newCell, pos);

                    mRendering->updateObjectCell(ptr, copy);
                    MWBase::Environment::get().getSoundManager()->updatePtr (ptr, copy);

                    MWBase::MechanicsManager *mechMgr = MWBase::Environment::get().getMechanicsManager();
                    mechMgr->updateCell(ptr, copy);

                    std::string script =
                        ptr.getClass().getScript(ptr);
                    if (!script.empty())
                    {
                        mLocalScripts.remove(ptr);
                        removeContainerScripts (ptr);
                        mLocalScripts.add(script, copy);
                        addContainerScripts (copy, newCell);
                    }
                }
                ptr.getRefData().setCount(0);
            }
        }
        if (haveToMove && ptr.getRefData().getBaseNode())
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

        moveObject(ptr, cell, x, y, z);

        return cell != ptr.getCell();
    }

    void World::moveObject (const Ptr& ptr, float x, float y, float z)
    {
        moveObjectImp(ptr, x, y, z);
    }

    void World::scaleObject (const Ptr& ptr, float scale)
    {
        ptr.getCellRef().setScale(scale);
        ptr.getClass().adjustScale(ptr,scale);

        if(ptr.getRefData().getBaseNode() == 0)
            return;
        mRendering->scaleObject(ptr, Vector3(scale,scale,scale));
        mPhysics->scaleObject(ptr);
    }

    void World::rotateObjectImp (const Ptr& ptr, Ogre::Vector3 rot, bool adjust)
    {
        const float two_pi = Ogre::Math::TWO_PI;
        const float pi = Ogre::Math::PI;

        ESM::Position pos = ptr.getRefData().getPosition();
        float *objRot = pos.rot;
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

        if(ptr.getClass().isActor())
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

        ptr.getRefData().setPosition(pos);

        if(ptr.getRefData().getBaseNode() == 0)
            return;

        if (ptr.getClass().isActor())
            mWorldScene->updateObjectRotation(ptr);
        else
            mWorldScene->updateObjectLocalRotation(ptr);
    }

    void World::localRotateObject (const Ptr& ptr, float x, float y, float z)
    {
        LocalRotation rot = ptr.getRefData().getLocalRotation();
        rot.rot[0]=Ogre::Degree(x).valueRadians();
        rot.rot[1]=Ogre::Degree(y).valueRadians();
        rot.rot[2]=Ogre::Degree(z).valueRadians();

        wrap(rot.rot[0]);
        wrap(rot.rot[1]);
        wrap(rot.rot[2]);

        ptr.getRefData().setLocalRotation(rot);

        if (ptr.getRefData().getBaseNode() != 0)
        {
            mWorldScene->updateObjectLocalRotation(ptr);
        }
    }

    void World::adjustPosition(const Ptr &ptr)
    {
        ESM::Position pos (ptr.getRefData().getPosition());

        if(!ptr.getRefData().getBaseNode())
        {
            // will be adjusted when Ptr's cell becomes active
            return;
        }

        float terrainHeight = mRendering->getTerrainHeightAt(Ogre::Vector3(pos.pos));

        if (pos.pos[2] < terrainHeight)
            pos.pos[2] = terrainHeight;

        pos.pos[2] += 20; // place slightly above. will snap down to ground with code below

        ptr.getRefData().setPosition(pos);

        if (!isFlying(ptr))
        {
            Ogre::Vector3 traced = mPhysics->traceDown(ptr, 200);
            if (traced.z < pos.pos[2])
                pos.pos[2] = traced.z;
        }

        moveObject(ptr, ptr.getCell(), pos.pos[0], pos.pos[1], pos.pos[2]);
    }

    void World::fixPosition(const Ptr &actor)
    {
        const float dist = 8000;
        ESM::Position pos (actor.getRefData().getPosition());
        pos.pos[2] += dist;
        actor.getRefData().setPosition(pos);

        Ogre::Vector3 traced = mPhysics->traceDown(actor, dist*1.1);
        moveObject(actor, actor.getCell(), traced.x, traced.y, traced.z);
    }

    void World::rotateObject (const Ptr& ptr,float x,float y,float z, bool adjust)
    {
        rotateObjectImp(ptr, Ogre::Vector3(Ogre::Degree(x).valueRadians(),
                                           Ogre::Degree(y).valueRadians(),
                                           Ogre::Degree(z).valueRadians()),
                        adjust);
    }

    MWWorld::Ptr World::safePlaceObject(const MWWorld::Ptr& ptr, MWWorld::CellStore* cell, ESM::Position pos)
    {
        return copyObjectToCell(ptr,cell,pos,false);
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

        mProjectileManager->update(duration);

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

        mPhysics->stepSimulation(duration);
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
            if (!mWorldScene->isCellActive(*it->first.getCell()) || !it->first.getRefData().getBaseNode())
            {
                // The door is no longer in an active cell, or it was disabled.
                // Erase from mDoorStates, since we no longer need to move it.
                // Once we load the door's cell again (or re-enable the door), Door::insertObject will reinsert to mDoorStates.
                mDoorStates.erase(it++);
            }
            else
            {
                float oldRot = Ogre::Radian(it->first.getRefData().getLocalRotation().rot[2]).valueDegrees();
                float diff = duration * 90;
                float targetRot = std::min(std::max(0.f, oldRot + diff * (it->second == 1 ? 1 : -1)), 90.f);
                localRotateObject(it->first, 0, 0, targetRot);

                bool reached = (targetRot == 90.f && it->second) || targetRot == 0.f;

                /// \todo should use convexSweepTest here
                std::vector<std::string> collisions = mPhysics->getCollisions(it->first);
                for (std::vector<std::string>::iterator cit = collisions.begin(); cit != collisions.end(); ++cit)
                {
                    MWWorld::Ptr ptr = getPtrViaHandle(*cit);
                    if (ptr.getClass().isActor())
                    {
                        // Collided with actor, ask actor to try to avoid door
                        if(ptr != MWBase::Environment::get().getWorld()->getPlayerPtr() ) {
                            MWMechanics::AiSequence& seq = ptr.getClass().getCreatureStats(ptr).getAiSequence();
                            if(seq.getTypeId() != MWMechanics::AiPackage::TypeIdAvoidDoor) //Only add it once
                                seq.stack(MWMechanics::AiAvoidDoor(it->first),ptr);
                        }

                        // we need to undo the rotation
                        localRotateObject(it->first, 0, 0, oldRot);
                        reached = false;
                        //break; //Removed in case multiple actors are touching
                    }
                }

                if (reached)
                {
                    // Mark as non-moving
                    it->first.getClass().setDoorState(it->first, 0);
                    mDoorStates.erase(it++);
                }
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

            mGlobalVariables["pcrace"].setInteger (i == ids.size() ? 0 : i+1);

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
        if (mGoToJail && !paused)
            goToJail();

        updateWeather(duration);

        if (!paused)
            doPhysics (duration);

        mWorldScene->update (duration, paused);

        performUpdateSceneQueries ();

        updateWindowManager ();

        if (!paused && mPlayer->getPlayer().getCell()->isExterior())
        {
            ESM::Position pos = mPlayer->getPlayer().getRefData().getPosition();
            mPlayer->setLastKnownExteriorPosition(Ogre::Vector3(pos.pos));
        }
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
            const float* p = mPlayer->getPlayer().getRefData().getPosition().pos;
            Vector3 sun = mRendering->getSkyManager()->getRealSunPos();
            mRendering->getSkyManager()->setGlare(!mPhysics->castRay(Ogre::Vector3(p[0], p[1], p[2]), sun));
        }

        updateFacedHandle ();
    }

    void World::getFacedHandle(std::string& facedHandle, float maxDistance)
    {
        maxDistance += mRendering->getCameraDistance();

        std::vector < std::pair < float, std::string > > results;
        if (MWBase::Environment::get().getWindowManager()->isGuiMode())
        {
            float x, y;
            MWBase::Environment::get().getWindowManager()->getMousePosition(x, y);
            results = mPhysics->getFacedHandles(x, y, maxDistance);
            if (MWBase::Environment::get().getWindowManager()->isConsoleMode())
                results = mPhysics->getFacedHandles(x, y, getMaxActivationDistance ()*50);
        }
        else
        {
            results = mPhysics->getFacedHandles(maxDistance);
        }

        // ignore the player and other things we're not interested in
        std::vector < std::pair < float, std::string > >::iterator it = results.begin();
        while (it != results.end())
        {
            if ((*it).second.find("HeightField") != std::string::npos) // Don't attempt to getPtrViaHandle on terrain
            {
                ++it;
                continue;
            }

            if (getPtrViaHandle((*it).second) == mPlayer->getPlayer() ) // not interested in player (unless you want to talk to yourself)
            {
                it = results.erase(it);
            }
            else
                ++it;
        }

        if (results.empty()
                || results.front().second.find("HeightField") != std::string::npos) // Blocked by terrain
            facedHandle = "";
        else
            facedHandle = results.front().second;
    }

    void World::updateFacedHandle ()
    {
        float telekinesisRangeBonus =
                mPlayer->getPlayer().getClass().getCreatureStats(mPlayer->getPlayer()).getMagicEffects()
                .get(ESM::MagicEffect::Telekinesis).mMagnitude;
        telekinesisRangeBonus = feetToGameUnits(telekinesisRangeBonus);

        float activationDistance = getMaxActivationDistance() + telekinesisRangeBonus;

        getFacedHandle(mFacedHandle, activationDistance);
    }

    bool World::isCellExterior() const
    {
        CellStore *currentCell = mWorldScene->getCurrentCell();
        if (currentCell)
        {
            return currentCell->getCell()->isExterior();
        }
        return false;
    }

    bool World::isCellQuasiExterior() const
    {
        CellStore *currentCell = mWorldScene->getCurrentCell();
        if (currentCell)
        {
            if (!(currentCell->getCell()->mData.mFlags & ESM::Cell::QuasiEx))
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
        MWWorld::CellRefList<ESM::Static>& statics = cell->get<ESM::Static>();
        MWWorld::LiveCellRef<ESM::Static>* ref = statics.find("northmarker");
        if (!ref)
            return Vector2(0, 1);
        Ogre::SceneNode* node = ref->mData.getBaseNode();
        Vector3 dir = node->_getDerivedOrientation() * Ogre::Vector3(0,1,0);
        Vector2 d = Vector2(dir.x, dir.y);
        return d;
    }

    void World::getDoorMarkers (CellStore* cell, std::vector<World::DoorMarker>& out)
    {
        MWWorld::CellRefList<ESM::Door>& doors = cell->get<ESM::Door>();
        CellRefList<ESM::Door>::List& refList = doors.mList;
        for (CellRefList<ESM::Door>::List::iterator it = refList.begin(); it != refList.end(); ++it)
        {
            MWWorld::LiveCellRef<ESM::Door>& ref = *it;

            if (!ref.mData.isEnabled())
                continue;

            if (ref.mRef.getTeleport())
            {
                World::DoorMarker newMarker;
                newMarker.name = MWClass::Door::getDestination(ref);

                ESM::Position pos = ref.mData.getPosition ();

                newMarker.x = pos.pos[0];
                newMarker.y = pos.pos[1];
                out.push_back(newMarker);
            }
        }
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

    bool World::toggleWater()
    {
        return mRendering->toggleWater();
    }

    void World::PCDropped (const Ptr& item)
    {
        std::string script = item.getClass().getScript(item);

        // Set OnPCDrop Variable on item's script, if it has a script with that variable declared
        if(script != "")
            item.getRefData().getLocals().setVarByInt(script, "onpcdrop", 1);
    }

    MWWorld::Ptr World::placeObject (const MWWorld::Ptr& object, float cursorX, float cursorY, int amount)
    {
        std::pair<bool, Ogre::Vector3> result = mPhysics->castRay(cursorX, cursorY);

        if (!result.first)
            return MWWorld::Ptr();

        CellStore* cell = getPlayerPtr().getCell();

        ESM::Position pos = getPlayerPtr().getRefData().getPosition();
        pos.pos[0] = result.second[0];
        pos.pos[1] = result.second[1];
        pos.pos[2] = result.second[2];
        // We want only the Z part of the player's rotation
        pos.rot[0] = 0;
        pos.rot[1] = 0;

        // copy the object and set its count
        int origCount = object.getRefData().getCount();
        object.getRefData().setCount(amount);
        Ptr dropped = copyObjectToCell(object, cell, pos, true);
        object.getRefData().setCount(origCount);

        // only the player place items in the world, so no need to check actor
        PCDropped(dropped);

        return dropped;
    }

    bool World::canPlaceObject(float cursorX, float cursorY)
    {
        Ogre::Vector3 normal(0,0,0);
        std::string handle;
        std::pair<bool, Ogre::Vector3> result = mPhysics->castRay(cursorX, cursorY, &normal, &handle);

        if (result.first)
        {
            // check if the wanted position is on a flat surface, and not e.g. against a vertical wall
            if (normal.angleBetween(Ogre::Vector3(0.f,0.f,1.f)).valueDegrees() >= 30)
                return false;

            MWWorld::Ptr hitObject = searchPtrViaHandle(handle);
            if (!hitObject.isEmpty() && hitObject.getClass().isActor())
                return false;

            return true;
        }
        else
            return false;
    }


    Ptr World::copyObjectToCell(const Ptr &object, CellStore* cell, ESM::Position pos, bool adjustPos)
    {
        if (!object.getClass().isActor() && adjustPos)
        {
            // Adjust position so the location we wanted ends up in the middle of the object bounding box
            Ogre::Vector3 min, max;
            if (mPhysics->getObjectAABB(object, min, max)) {
                Ogre::Quaternion xr(Ogre::Radian(-pos.rot[0]), Ogre::Vector3::UNIT_X);
                Ogre::Quaternion yr(Ogre::Radian(-pos.rot[1]), Ogre::Vector3::UNIT_Y);
                Ogre::Quaternion zr(Ogre::Radian(-pos.rot[2]), Ogre::Vector3::UNIT_Z);

                Ogre::Vector3 adjust (
                             (min.x + max.x) / 2,
                            (min.y + max.y) / 2,
                            min.z
                            );
                adjust = (xr*yr*zr) * adjust;
                pos.pos[0] -= adjust.x;
                pos.pos[1] -= adjust.y;
                pos.pos[2] -= adjust.z;
            }
        }

        if (cell->isExterior())
        {
            int cellX, cellY;
            positionToIndex(pos.pos[0], pos.pos[1], cellX, cellY);
            cell = mCells.getExterior(cellX, cellY);
        }

        MWWorld::Ptr dropped =
            object.getClass().copyToCell(object, *cell, pos);

        // Reset some position values that could be uninitialized if this item came from a container
        LocalRotation localRotation;
        localRotation.rot[0] = 0;
        localRotation.rot[1] = 0;
        localRotation.rot[2] = 0;
        dropped.getRefData().setLocalRotation(localRotation);
        dropped.getCellRef().setPosition(pos);
        dropped.getCellRef().unsetRefNum();

        if (mWorldScene->isCellActive(*cell)) {
            if (dropped.getRefData().isEnabled()) {
                mWorldScene->addObjectToScene(dropped);
            }
            std::string script = dropped.getClass().getScript(dropped);
            if (!script.empty()) {
                mLocalScripts.add(script, dropped);
            }
            addContainerScripts(dropped, cell);
        }

        return dropped;
    }

    MWWorld::Ptr World::dropObjectOnGround (const Ptr& actor, const Ptr& object, int amount)
    {
        MWWorld::CellStore* cell = actor.getCell();

        ESM::Position pos =
            actor.getRefData().getPosition();
        // We want only the Z part of the actor's rotation
        pos.rot[0] = 0;
        pos.rot[1] = 0;

        Ogre::Vector3 orig =
            Ogre::Vector3(pos.pos);
        orig.z += 20;
        Ogre::Vector3 dir = Ogre::Vector3(0, 0, -1);

        float len = 100.0;

        std::pair<bool, Ogre::Vector3> hit =
            mPhysics->castRay(orig, dir, len);
        if (hit.first)
            pos.pos[2] = hit.second.z;

        // copy the object and set its count
        int origCount = object.getRefData().getCount();
        object.getRefData().setCount(amount);
        Ptr dropped = copyObjectToCell(object, cell, pos);
        object.getRefData().setCount(origCount);

        if(actor == mPlayer->getPlayer()) // Only call if dropped by player
            PCDropped(dropped);
        return dropped;
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
        const MWMechanics::CreatureStats &stats = ptr.getClass().getCreatureStats(ptr);
        bool isParalyzed = (stats.getMagicEffects().get(ESM::MagicEffect::Paralyze).mMagnitude > 0);

        if(!ptr.getClass().isActor())
            return false;

        if (ptr.getClass().getCreatureStats(ptr).isDead())
            return false;

        if (ptr.getClass().canFly(ptr))
            return !isParalyzed;

        if(stats.getMagicEffects().get(ESM::MagicEffect::Levitate).mMagnitude > 0
                && isLevitationEnabled())
            return true;

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
        if(stats.getMagicEffects().get(ESM::MagicEffect::SlowFall).mMagnitude > 0)
            return true;

        return false;
    }

    bool World::isSubmerged(const MWWorld::Ptr &object) const
    {
        const float *fpos = object.getRefData().getPosition().pos;
        Ogre::Vector3 pos(fpos[0], fpos[1], fpos[2]);

        const OEngine::Physic::PhysicActor *actor = mPhysEngine->getCharacter(object.getRefData().getHandle());
        if(actor) pos.z += 1.85*actor->getHalfExtents().z;

        return isUnderwater(object.getCell(), pos);
    }

    bool
    World::isSwimming(const MWWorld::Ptr &object) const
    {
        /// \todo add check ifActor() - only actors can swim
        const float *fpos = object.getRefData().getPosition().pos;
        Ogre::Vector3 pos(fpos[0], fpos[1], fpos[2]);

        /// \fixme 3/4ths submerged?
        const OEngine::Physic::PhysicActor *actor = mPhysEngine->getCharacter(object.getRefData().getHandle());
        if(actor) pos.z += actor->getHalfExtents().z * 1.5;

        return isUnderwater(object.getCell(), pos);
    }

    bool
    World::isUnderwater(const MWWorld::CellStore* cell, const Ogre::Vector3 &pos) const
    {
        if (!(cell->getCell()->mData.mFlags & ESM::Cell::HasWater)) {
            return false;
        }
        return pos.z < cell->getWaterLevel();
    }

    // physactor->getOnGround() is not a reliable indicator of whether the actor
    // is on the ground (defaults to false, which means code blocks such as
    // CharacterController::update() may falsely detect "falling").
    //
    // Also, collisions can move z position slightly off zero, giving a false
    // indication. In order to reduce false detection of jumping, small distance
    // below the actor is detected and ignored. A value of 1.5 is used here, but
    // something larger may be more suitable.  This change should resolve Bug#1271.
    //
    // TODO: There might be better places to update PhysicActor::mOnGround.
    bool World::isOnGround(const MWWorld::Ptr &ptr) const
    {
        RefData &refdata = ptr.getRefData();
        const OEngine::Physic::PhysicActor *physactor = mPhysEngine->getCharacter(refdata.getHandle());

        if(!physactor)
            return false;

        if(physactor->getOnGround())
            return true;
        else
        {
            Ogre::Vector3 pos(ptr.getRefData().getPosition().pos);
            OEngine::Physic::ActorTracer tracer;
            // a small distance above collision object is considered "on ground"
            tracer.findGround(physactor,
                              pos,
                              pos - Ogre::Vector3(0, 0, 1.5f), // trace a small amount down
                              mPhysEngine);
            if(tracer.mFraction < 1.0f) // collision, must be close to something below
            {
                const_cast<OEngine::Physic::PhysicActor *> (physactor)->setOnGround(true);
                return true;
            }
            else
                return false;
        }
    }

    bool World::vanityRotateCamera(float * rot)
    {
        return mRendering->vanityRotateCamera(rot);
    }

    void World::setCameraDistance(float dist, bool adjust, bool override_)
    {
        return mRendering->setCameraDistance(dist, adjust, override_);
    }

    void World::setupPlayer()
    {
        const ESM::NPC *player = mStore.get<ESM::NPC>().find("player");
        if (!mPlayer)
            mPlayer = new MWWorld::Player(player, *this);
        else
        {
            // Remove the old CharacterController
            MWBase::Environment::get().getMechanicsManager()->remove(getPlayerPtr());

            mPlayer->set(player);
        }

        Ptr ptr = mPlayer->getPlayer();
        mRendering->setupPlayer(ptr);
    }

    void World::renderPlayer()
    {
        mRendering->renderPlayer(mPlayer->getPlayer());

        // At this point the Animation object is live, and the CharacterController associated with it must be created.
        // It has to be done at this point: resetCamera below does animation->setViewMode -> CharacterController::forceStateUpdate
        // so we should make sure not to use a "stale" controller for that.
        MWBase::Environment::get().getMechanicsManager()->add(mPlayer->getPlayer());

        mPhysics->addActor(mPlayer->getPlayer());
        mRendering->resetCamera();
    }

    int World::canRest ()
    {
        CellStore *currentCell = mWorldScene->getCurrentCell();

        Ptr player = mPlayer->getPlayer();
        RefData &refdata = player.getRefData();
        Ogre::Vector3 playerPos(refdata.getPosition().pos);

        const OEngine::Physic::PhysicActor *physactor = mPhysEngine->getCharacter(refdata.getHandle());
        if((!physactor->getOnGround()&&physactor->getCollisionMode()) || isUnderwater(currentCell, playerPos))
            return 2;
        if((currentCell->getCell()->mData.mFlags&ESM::Cell::NoSleep) ||
           player.getClass().getNpcStats(player).isWerewolf())
            return 1;

        return 0;
    }

    MWRender::Animation* World::getAnimation(const MWWorld::Ptr &ptr)
    {
        return mRendering->getAnimation(ptr);
    }

    void World::frameStarted (float dt, bool paused)
    {
        mRendering->frameStarted(dt, paused);
    }

    void World::screenshot(Ogre::Image &image, int w, int h)
    {
        mRendering->screenshot(image, w, h);
    }

    void World::activateDoor(const MWWorld::Ptr& door)
    {
        int state = door.getClass().getDoorState(door);
        switch (state)
        {
        case 0:
            if (door.getRefData().getLocalRotation().rot[2] == 0)
                state = 1; // if closed, then open
            else
                state = 2; // if open, then close
            break;
        case 2:
            state = 1; // if closing, then open
            break;
        case 1:
        default:
            state = 2; // if opening, then close
            break;
        }
        door.getClass().setDoorState(door, state);
        mDoorStates[door] = state;
    }

    void World::activateDoor(const Ptr &door, int state)
    {
        door.getClass().setDoorState(door, state);
        mDoorStates[door] = state;
        if (state == 0)
            mDoorStates.erase(door);
    }

    bool World::getPlayerStandingOn (const MWWorld::Ptr& object)
    {
        MWWorld::Ptr player = getPlayerPtr();
        return mPhysics->isActorStandingOn(player, object);
    }

    bool World::getActorStandingOn (const MWWorld::Ptr& object)
    {
        std::vector<std::string> actors;
        mPhysics->getActorsStandingOn(object, actors);
        return !actors.empty();
    }

    bool World::getPlayerCollidingWith (const MWWorld::Ptr& object)
    {
        MWWorld::Ptr player = getPlayerPtr();
        return mPhysics->isActorCollidingWith(player, object);
    }

    bool World::getActorCollidingWith (const MWWorld::Ptr& object)
    {
        std::vector<std::string> actors;
        mPhysics->getActorsCollidingWith(object, actors);
        return !actors.empty();
    }

    void World::hurtStandingActors(const Ptr &object, float healthPerSecond)
    {
        std::vector<std::string> actors;
        mPhysics->getActorsStandingOn(object, actors);
        for (std::vector<std::string>::iterator it = actors.begin(); it != actors.end(); ++it)
        {
            MWWorld::Ptr actor = searchPtrViaHandle(*it); // Collision events are from the last frame, actor might no longer exist
            if (actor.isEmpty())
                continue;

            MWMechanics::CreatureStats& stats = actor.getClass().getCreatureStats(actor);
            MWMechanics::DynamicStat<float> health = stats.getHealth();
            health.setCurrent(health.getCurrent()-healthPerSecond*MWBase::Environment::get().getFrameDuration());
            stats.setHealth(health);
        }
    }

    void World::hurtCollidingActors(const Ptr &object, float healthPerSecond)
    {
        std::vector<std::string> actors;
        mPhysics->getActorsCollidingWith(object, actors);
        for (std::vector<std::string>::iterator it = actors.begin(); it != actors.end(); ++it)
        {
            MWWorld::Ptr actor = searchPtrViaHandle(*it); // Collision events are from the last frame, actor might no longer exist
            if (actor.isEmpty())
                continue;

            MWMechanics::CreatureStats& stats = actor.getClass().getCreatureStats(actor);
            MWMechanics::DynamicStat<float> health = stats.getHealth();
            health.setCurrent(health.getCurrent()-healthPerSecond*MWBase::Environment::get().getFrameDuration());
            stats.setHealth(health);
        }
    }

    float World::getWindSpeed()
    {
        if (isCellExterior() || isCellQuasiExterior())
            return mWeatherManager->getWindSpeed();
        else
            return 0.f;
    }

    bool World::isInStorm() const
    {
        if (isCellExterior() || isCellQuasiExterior())
            return mWeatherManager->isInStorm();
        else
            return false;
    }

    Ogre::Vector3 World::getStormDirection() const
    {
        if (isCellExterior() || isCellQuasiExterior())
            return mWeatherManager->getStormDirection();
        else
            return Ogre::Vector3(0,1,0);
    }

    void World::getContainersOwnedBy (const MWWorld::Ptr& npc, std::vector<MWWorld::Ptr>& out)
    {
        const Scene::CellStoreCollection& collection = mWorldScene->getActiveCells();
        for (Scene::CellStoreCollection::const_iterator cellIt = collection.begin(); cellIt != collection.end(); ++cellIt)
        {
            MWWorld::CellRefList<ESM::Container>& containers = (*cellIt)->get<ESM::Container>();
            CellRefList<ESM::Container>::List& refList = containers.mList;
            for (CellRefList<ESM::Container>::List::iterator container = refList.begin(); container != refList.end(); ++container)
            {
                MWWorld::Ptr ptr (&*container, *cellIt);
                if (Misc::StringUtils::ciEqual(ptr.getCellRef().getOwner(), npc.getCellRef().getRefId()))
                    out.push_back(ptr);
            }
        }
    }

    struct ListHandlesFunctor
    {
        std::vector<std::string> mHandles;

        bool operator() (Ptr ptr)
        {
            Ogre::SceneNode* handle = ptr.getRefData().getBaseNode();
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
                if (Misc::StringUtils::ciEqual(searchPtrViaHandle(*it).getCellRef().getOwner(), npc.getCellRef().getRefId()))
                    out.push_back(searchPtrViaHandle(*it));
        }
    }

    bool World::getLOS(const MWWorld::Ptr& actor,const MWWorld::Ptr& targetActor)
    {
        if (!targetActor.getRefData().isEnabled() || !actor.getRefData().isEnabled())
            return false; // cannot get LOS unless both NPC's are enabled
        if (!targetActor.getRefData().getBaseNode() || !targetActor.getRefData().getBaseNode())
            return false; // not in active cell

        Ogre::Vector3 halfExt1 = mPhysEngine->getCharacter(actor.getRefData().getHandle())->getHalfExtents();
        const float* pos1 = actor.getRefData().getPosition().pos;
        Ogre::Vector3 halfExt2 = mPhysEngine->getCharacter(targetActor.getRefData().getHandle())->getHalfExtents();
        const float* pos2 = targetActor.getRefData().getPosition().pos;

        btVector3 from(pos1[0],pos1[1],pos1[2]+halfExt1.z*2*0.9); // eye level
        btVector3 to(pos2[0],pos2[1],pos2[2]+halfExt2.z*2*0.9);

        std::pair<std::string, float> result = mPhysEngine->rayTest(from, to,false);
        if(result.first == "") return true;

        return false;
    }

    float World::getDistToNearestRayHit(const Ogre::Vector3& from, const Ogre::Vector3& dir, float maxDist)
    {
        btVector3 btFrom(from.x, from.y, from.z);
        btVector3 btTo = btVector3(dir.x, dir.y, dir.z);
        btTo.normalize();
        btTo = btFrom + btTo * maxDist;

        std::pair<std::string, float> result = mPhysEngine->rayTest(btFrom, btTo, false);

        if(result.second == -1) return maxDist;
        else return result.second*(btTo-btFrom).length();
    }

    void World::enableActorCollision(const MWWorld::Ptr& actor, bool enable)
    {
        OEngine::Physic::PhysicActor *physicActor = mPhysEngine->getCharacter(actor.getRefData().getHandle());
        if (physicActor)
            physicActor->enableCollisionBody(enable);
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
        const DoorList &doors = cellStore->get<ESM::Door>().mList;
        for (DoorList::const_iterator it = doors.begin(); it != doors.end(); ++it) {
            if (!it->mRef.getTeleport()) {
                continue;
            }

            MWWorld::CellStore *source = 0;

            // door to exterior
            if (it->mRef.getDestCell().empty()) {
                int x, y;
                ESM::Position doorDest = it->mRef.getDoorDest();
                positionToIndex(doorDest.pos[0], doorDest.pos[1], x, y);
                source = getExterior(x, y);
            }
            // door to interior
            else {
                source = getInterior(it->mRef.getDestCell());
            }
            if (0 != source) {
                // Find door leading to our current teleport door
                // and use it destination to position inside cell.
                const DoorList &doors = source->get<ESM::Door>().mList;
                for (DoorList::const_iterator jt = doors.begin(); jt != doors.end(); ++jt) {
                    if (it->mRef.getTeleport() &&
                        Misc::StringUtils::ciEqual(name, jt->mRef.getDestCell()))
                    {
                        /// \note Using _any_ door pointed to the interior,
                        /// not the one pointed to current door.
                        pos = jt->mRef.getDoorDest();
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

            // Note: Z pos will be adjusted by adjustPosition later
            pos.pos[2] = 0;

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
        MWMechanics::NpcStats& npcStats = actor.getClass().getNpcStats(actor);

        // The actor does not have to change state
        if (npcStats.isWerewolf() == werewolf)
            return;

        npcStats.setWerewolf(werewolf);

        // This is a bit dangerous. Equipped items other than WerewolfRobe may reference
        // bones that do not even exist with the werewolf object root.
        // Therefore, make sure to unequip everything at once, and only fire the change event
        // (which will rebuild the animation parts) afterwards. unequipAll will do this for us.
        MWWorld::InventoryStore& invStore = actor.getClass().getInventoryStore(actor);
        invStore.unequipAll(actor);

        if(werewolf)
        {
            InventoryStore &inv = actor.getClass().getInventoryStore(actor);

            inv.equip(InventoryStore::Slot_Robe, inv.ContainerStore::add("werewolfrobe", 1, actor), actor);
        }
        else
        {
            actor.getClass().getContainerStore(actor).remove("werewolfrobe", 1, actor);
        }

        // NpcAnimation::updateParts will already rebuild the animation when it detects change of Npc type.
        // the following is just for reattaching the camera properly.
        mRendering->rebuildPtr(actor);

        if(actor.getRefData().getHandle() == "player")
        {
            // Update the GUI only when called on the player
            MWBase::WindowManager* windowManager = MWBase::Environment::get().getWindowManager();

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
    }

    void World::applyWerewolfAcrobatics(const Ptr &actor)
    {
        const Store<ESM::GameSetting> &gmst = getStore().get<ESM::GameSetting>();
        MWMechanics::NpcStats &stats = actor.getClass().getNpcStats(actor);

        stats.getSkill(ESM::Skill::Acrobatics).setBase(gmst.find("fWerewolfAcrobatics")->getFloat());
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
            else
            {
                std::stringstream msg;
                msg << "Failed loading " << *it << ": the content file does not exist";
                throw std::runtime_error(msg.str());
            }
        }
    }

    bool World::startSpellCast(const Ptr &actor)
    {
        MWMechanics::CreatureStats& stats = actor.getClass().getCreatureStats(actor);

        std::string message;
        bool fail = false;
        bool isPlayer = (actor == getPlayerPtr());

        std::string selectedSpell = stats.getSpells().getSelectedSpell();

        if (!selectedSpell.empty())
        {
            const ESM::Spell* spell = getStore().get<ESM::Spell>().search(selectedSpell);

            // Check mana
            MWMechanics::DynamicStat<float> magicka = stats.getMagicka();
            if (magicka.getCurrent() < spell->mData.mCost)
            {
                message = "#{sMagicInsufficientSP}";
                fail = true;
            }

            // If this is a power, check if it was already used in the last 24h
            if (!fail && spell->mData.mType == ESM::Spell::ST_Power && !stats.getSpells().canUsePower(spell->mId))
            {
                message = "#{sPowerAlreadyUsed}";
                fail = true;
            }

            // Reduce mana
            if (!fail)
            {
                magicka.setCurrent(magicka.getCurrent() - spell->mData.mCost);
                stats.setMagicka(magicka);
            }
        }

        if (isPlayer && fail)
            MWBase::Environment::get().getWindowManager()->messageBox(message);

        return !fail;
    }

    void World::castSpell(const Ptr &actor)
    {
        MWMechanics::CreatureStats& stats = actor.getClass().getCreatureStats(actor);

        // Get the target to use for "on touch" effects
        MWWorld::Ptr target;
        float distance = 192.f; // ??

        if (actor == getPlayerPtr())
        {
            // For the player, use camera to aim
            std::string facedHandle;
            getFacedHandle(facedHandle, distance);
            if (!facedHandle.empty())
                target = getPtrViaHandle(facedHandle);
        }
        else
        {
            // For NPCs use facing direction from Head node
            Ogre::Vector3 origin(actor.getRefData().getPosition().pos);
            MWRender::Animation *anim = mRendering->getAnimation(actor);
            if(anim != NULL)
            {
                Ogre::Node *node = anim->getNode("Head");
                if (node == NULL)
                    node = anim->getNode("Bip01 Head");
                if(node != NULL)
                    origin += node->_getDerivedPosition();
            }
            Ogre::Quaternion orient;
            orient = Ogre::Quaternion(Ogre::Radian(actor.getRefData().getPosition().rot[2]), Ogre::Vector3::NEGATIVE_UNIT_Z) *
                    Ogre::Quaternion(Ogre::Radian(actor.getRefData().getPosition().rot[0]), Ogre::Vector3::NEGATIVE_UNIT_X);
            Ogre::Vector3 direction = orient.yAxis();
            Ogre::Vector3 dest = origin + direction * distance;


            std::vector<std::pair<float, std::string> > collisions = mPhysEngine->rayTest2(btVector3(origin.x, origin.y, origin.z), btVector3(dest.x, dest.y, dest.z));
            for (std::vector<std::pair<float, std::string> >::iterator cIt = collisions.begin(); cIt != collisions.end(); ++cIt)
            {
                MWWorld::Ptr collided = getPtrViaHandle(cIt->second);
                if (collided != actor)
                {
                    target = collided;
                    break;
                }
            }
        }

        std::string selectedSpell = stats.getSpells().getSelectedSpell();

        MWMechanics::CastSpell cast(actor, target);
        if (!target.isEmpty())
            cast.mHitPosition = Ogre::Vector3(target.getRefData().getPosition().pos);

        if (!selectedSpell.empty())
        {
            const ESM::Spell* spell = getStore().get<ESM::Spell>().search(selectedSpell);
	    
	    // A power can be used once per 24h
	    if (spell->mData.mType == ESM::Spell::ST_Power)
	        stats.getSpells().usePower(spell->mId);

            cast.cast(spell);
        }
        else if (actor.getClass().hasInventoryStore(actor))
        {
            MWWorld::InventoryStore& inv = actor.getClass().getInventoryStore(actor);
            if (inv.getSelectedEnchantItem() != inv.end())
                cast.cast(*inv.getSelectedEnchantItem());
        }
    }

    void World::launchProjectile (MWWorld::Ptr actor, MWWorld::Ptr projectile,
                                   const Ogre::Vector3& worldPos, const Ogre::Quaternion& orient, MWWorld::Ptr bow, float speed)
    {
        mProjectileManager->launchProjectile(actor, projectile, worldPos, orient, bow, speed);
    }

    void World::launchMagicBolt (const std::string& model, const std::string &sound, const std::string &spellId,
                                 float speed, bool stack, const ESM::EffectList& effects,
                                   const MWWorld::Ptr& caster, const std::string& sourceName, const Ogre::Vector3& fallbackDirection)
    {
        mProjectileManager->launchMagicBolt(model, sound, spellId, speed, stack, effects, caster, sourceName, fallbackDirection);
    }

    const std::vector<std::string>& World::getContentFiles() const
    {
        return mContentFiles;
    }

    void World::breakInvisibility(const Ptr &actor)
    {
        actor.getClass().getCreatureStats(actor).getActiveSpells().purgeEffect(ESM::MagicEffect::Invisibility);
        if (actor.getClass().hasInventoryStore(actor))
            actor.getClass().getInventoryStore(actor).purgeEffect(ESM::MagicEffect::Invisibility);

        // Normally updated once per frame, but here it is kinda important to do it right away.
        MWBase::Environment::get().getMechanicsManager()->updateMagicEffects(actor);
    }

    bool World::isDark() const
    {
        MWWorld::CellStore* cell = mPlayer->getPlayer().getCell();
        if (cell->isExterior())
            return mWeatherManager->isDark();
        else
        {
            uint32_t ambient = cell->getCell()->mAmbi.mAmbient;
            int ambientTotal = (ambient & 0xff)
                    + ((ambient>>8) & 0xff)
                    + ((ambient>>16) & 0xff);
            return !(cell->getCell()->mData.mFlags & ESM::Cell::NoSleep) && ambientTotal <= 201;
        }
    }

    bool World::findInteriorPositionInWorldSpace(MWWorld::CellStore* cell, Ogre::Vector3& result)
    {
        if (cell->isExterior())
            return false;
        MWWorld::CellRefList<ESM::Door>& doors = cell->get<ESM::Door>();
        CellRefList<ESM::Door>::List& refList = doors.mList;

        // Check if any door in the cell leads to an exterior directly
        for (CellRefList<ESM::Door>::List::iterator it = refList.begin(); it != refList.end(); ++it)
        {
            MWWorld::LiveCellRef<ESM::Door>& ref = *it;
            if (ref.mRef.getTeleport() && ref.mRef.getDestCell().empty())
            {
                ESM::Position pos = ref.mRef.getDoorDest();
                result = Ogre::Vector3(pos.pos);
                return true;
            }
        }

        // No luck :(
        return false;
    }

    void World::teleportToClosestMarker (const MWWorld::Ptr& ptr,
                                          const std::string& id)
    {
        Ogre::Vector3 worldPos;
        if (!findInteriorPositionInWorldSpace(ptr.getCell(), worldPos))
            worldPos = mPlayer->getLastKnownExteriorPosition();

        MWWorld::Ptr closestMarker;
        float closestDistance = FLT_MAX;

        std::vector<MWWorld::Ptr> markers;
        mCells.getExteriorPtrs(id, markers);

        for (std::vector<MWWorld::Ptr>::iterator it = markers.begin(); it != markers.end(); ++it)
        {
            ESM::Position pos = it->getRefData().getPosition();
            Ogre::Vector3 markerPos = Ogre::Vector3(pos.pos);
            float distance = worldPos.squaredDistance(markerPos);
            if (distance < closestDistance)
            {
                closestDistance = distance;
                closestMarker = *it;
            }

        }

        MWWorld::ActionTeleport action("", closestMarker.getRefData().getPosition());
        action.execute(ptr);
    }

    void World::updateWeather(float duration)
    {
        if (mPlayer->wasTeleported())
        {
            mPlayer->setTeleported(false);
            mWeatherManager->switchToNextWeather(true);
        }

        mWeatherManager->update(duration);
    }

    struct AddDetectedReference
    {
        AddDetectedReference(std::vector<Ptr>& out, Ptr detector, World::DetectionType type, float squaredDist)
            : mOut(out), mDetector(detector), mType(type), mSquaredDist(squaredDist)
        {
        }

        std::vector<Ptr>& mOut;
        Ptr mDetector;
        float mSquaredDist;
        World::DetectionType mType;
        bool operator() (MWWorld::Ptr ptr)
        {
            if (Ogre::Vector3(ptr.getRefData().getPosition().pos).squaredDistance(
                        Ogre::Vector3(mDetector.getRefData().getPosition().pos)) >= mSquaredDist)
                return true;

            if (!ptr.getRefData().isEnabled())
                return true;

            // Consider references inside containers as well (except if we are looking for a Creature, they cannot be in containers)
            if (mType != World::Detect_Creature &&
                    (ptr.getClass().isActor() || ptr.getClass().getTypeName() == typeid(ESM::Container).name()))
            {
                MWWorld::ContainerStore& store = ptr.getClass().getContainerStore(ptr);
                {
                    for (MWWorld::ContainerStoreIterator it = store.begin(); it != store.end(); ++it)
                    {
                        if (needToAdd(*it, mDetector))
                        {
                            mOut.push_back(ptr);
                            return true;
                        }
                    }
                }
            }

            if (needToAdd(ptr, mDetector))
                mOut.push_back(ptr);

            return true;
        }

        bool needToAdd (MWWorld::Ptr ptr, MWWorld::Ptr detector)
        {
            if (mType == World::Detect_Creature)
            {
                // If in werewolf form, this detects only NPCs, otherwise only creatures
                if (detector.getClass().isNpc() && detector.getClass().getNpcStats(detector).isWerewolf())
                {
                    if (ptr.getClass().getTypeName() != typeid(ESM::NPC).name())
                        return false;
                }
                else if (ptr.getClass().getTypeName() != typeid(ESM::Creature).name())
                    return false;
            }
            if (mType == World::Detect_Key && !ptr.getClass().isKey(ptr))
                return false;
            if (mType == World::Detect_Enchantment && ptr.getClass().getEnchantment(ptr).empty())
                return false;
            return true;
        }
    };

    void World::listDetectedReferences(const Ptr &ptr, std::vector<Ptr> &out, DetectionType type)
    {
        const MWMechanics::MagicEffects& effects = ptr.getClass().getCreatureStats(ptr).getMagicEffects();
        float dist=0;
        if (type == World::Detect_Creature)
            dist = effects.get(ESM::MagicEffect::DetectAnimal).mMagnitude;
        else if (type == World::Detect_Key)
            dist = effects.get(ESM::MagicEffect::DetectKey).mMagnitude;
        else if (type == World::Detect_Enchantment)
            dist = effects.get(ESM::MagicEffect::DetectEnchantment).mMagnitude;

        if (!dist)
            return;

        dist = feetToGameUnits(dist);

        AddDetectedReference functor (out, ptr, type, dist*dist);

        const Scene::CellStoreCollection& active = mWorldScene->getActiveCells();
        for (Scene::CellStoreCollection::const_iterator it = active.begin(); it != active.end(); ++it)
        {
            MWWorld::CellStore* cellStore = *it;
            cellStore->forEach(functor);
        }
    }

    float World::feetToGameUnits(float feet)
    {
        // Looks like there is no GMST for this. This factor was determined in experiments
        // with the Telekinesis effect.
        return feet * 22;
    }

    MWWorld::Ptr World::getPlayerPtr()
    {
        return mPlayer->getPlayer();
    }

    void World::updateDialogueGlobals()
    {
        MWWorld::Ptr player = getPlayerPtr();
        int bounty = player.getClass().getNpcStats(player).getBounty();
        int playerGold = player.getClass().getContainerStore(player).count(ContainerStore::sGoldId);

        float fCrimeGoldDiscountMult = getStore().get<ESM::GameSetting>().find("fCrimeGoldDiscountMult")->getFloat();
        float fCrimeGoldTurnInMult = getStore().get<ESM::GameSetting>().find("fCrimeGoldTurnInMult")->getFloat();

        int discount = bounty * fCrimeGoldDiscountMult;
        int turnIn = bounty * fCrimeGoldTurnInMult;

        if (bounty > 0)
        {
            discount = std::max(1, discount);
            turnIn = std::max(1, turnIn);
        }

        mGlobalVariables["pchascrimegold"].setInteger((bounty <= playerGold) ? 1 : 0);

        mGlobalVariables["pchasgolddiscount"].setInteger((discount <= playerGold) ? 1 : 0);
        mGlobalVariables["crimegolddiscount"].setInteger(discount);

        mGlobalVariables["crimegoldturnin"].setInteger(turnIn);
        mGlobalVariables["pchasturnin"].setInteger((turnIn <= playerGold) ? 1 : 0);
    }

    void World::confiscateStolenItems(const Ptr &ptr)
    {
        Ogre::Vector3 playerPos;
        if (!findInteriorPositionInWorldSpace(ptr.getCell(), playerPos))
            playerPos = mPlayer->getLastKnownExteriorPosition();

        MWWorld::Ptr closestChest;
        float closestDistance = FLT_MAX;

        //Find closest stolen_goods chest
        std::vector<MWWorld::Ptr> chests;
        mCells.getInteriorPtrs("stolen_goods", chests);

        Ogre::Vector3 chestPos;
        for (std::vector<MWWorld::Ptr>::iterator it = chests.begin(); it != chests.end(); ++it)
        {
            if (!findInteriorPositionInWorldSpace(it->getCell(), chestPos))
                continue;

            float distance = playerPos.squaredDistance(chestPos);
            if (distance < closestDistance)
            {
                closestDistance = distance;
                closestChest = *it;
            }
        }

        if (!closestChest.isEmpty()) //Found a close chest
        {
            ContainerStore& store = ptr.getClass().getContainerStore(ptr);
            for (ContainerStoreIterator it = store.begin(); it != store.end(); ++it) //Move all stolen stuff into chest
            {
                if (!it->getCellRef().getOwner().empty() && it->getCellRef().getOwner() != "player") //Not owned by no one/player?
                {
                    closestChest.getClass().getContainerStore(closestChest).add(*it, it->getRefData().getCount(), closestChest);
                    store.remove(*it, it->getRefData().getCount(), ptr);
                }
            }
            closestChest.getClass().lock(closestChest,50);
        }
    }

    void World::goToJail()
    {
        if (!mGoToJail)
        {
            // Save for next update, since the player should be able to read the dialog text first
            mGoToJail = true;
            return;
        }
        else
        {
            mGoToJail = false;

            MWBase::Environment::get().getWindowManager()->removeGuiMode(MWGui::GM_Dialogue);

            MWWorld::Ptr player = getPlayerPtr();
            teleportToClosestMarker(player, "prisonmarker");
            int bounty = player.getClass().getNpcStats(player).getBounty();
            player.getClass().getNpcStats(player).setBounty(0);
            confiscateStolenItems(player);

            int iDaysinPrisonMod = getStore().get<ESM::GameSetting>().find("iDaysinPrisonMod")->getInt();
            int days = std::max(1, bounty / iDaysinPrisonMod);

            advanceTime(days * 24);
            for (int i=0; i<days*24; ++i)
                MWBase::Environment::get().getMechanicsManager ()->rest (true);

            std::set<int> skills;
            for (int day=0; day<days; ++day)
            {
                int skill = std::rand()/ (static_cast<double> (RAND_MAX) + 1) * ESM::Skill::Length;
                skills.insert(skill);

                MWMechanics::SkillValue& value = player.getClass().getNpcStats(player).getSkill(skill);
                if (skill == ESM::Skill::Security || skill == ESM::Skill::Sneak)
                    value.setBase(std::min(100, value.getBase()+1));
                else
                    value.setBase(value.getBase()-1);
            }

            const Store<ESM::GameSetting>& gmst = getStore().get<ESM::GameSetting>();

            std::string message;
            if (days == 1)
                message = gmst.find("sNotifyMessage42")->getString();
            else
                message = gmst.find("sNotifyMessage43")->getString();

            std::stringstream dayStr;
            dayStr << days;
            if (message.find("%d") != std::string::npos)
                message.replace(message.find("%d"), 2, dayStr.str());

            for (std::set<int>::iterator it = skills.begin(); it != skills.end(); ++it)
            {
                std::string skillName = gmst.find(ESM::Skill::sSkillNameIds[*it])->getString();
                std::stringstream skillValue;
                skillValue << player.getClass().getNpcStats(player).getSkill(*it).getBase();
                std::string skillMsg = gmst.find("sNotifyMessage44")->getString();
                if (*it == ESM::Skill::Sneak || *it == ESM::Skill::Security)
                    skillMsg = gmst.find("sNotifyMessage39")->getString();

                if (skillMsg.find("%s") != std::string::npos)
                    skillMsg.replace(skillMsg.find("%s"), 2, skillName);
                if (skillMsg.find("%d") != std::string::npos)
                    skillMsg.replace(skillMsg.find("%d"), 2, skillValue.str());
                message += "\n" + skillMsg;
            }

            std::vector<std::string> buttons;
            buttons.push_back("#{sOk}");
            MWBase::Environment::get().getWindowManager()->messageBox(message, buttons);
        }
    }

    void World::spawnRandomCreature(const std::string &creatureList)
    {
        const ESM::CreatureLevList* list = getStore().get<ESM::CreatureLevList>().find(creatureList);

        int iNumberCreatures = getStore().get<ESM::GameSetting>().find("iNumberCreatures")->getInt();
        int numCreatures = 1 + std::rand()/ (static_cast<double> (RAND_MAX) + 1) * iNumberCreatures; // [1, iNumberCreatures]

        for (int i=0; i<numCreatures; ++i)
        {
            std::string selectedCreature = MWMechanics::getLevelledItem(list, true);
            if (selectedCreature.empty())
                return;

            ESM::Position ipos = mPlayer->getPlayer().getRefData().getPosition();
            Ogre::Vector3 pos(ipos.pos);
            Ogre::Quaternion rot(Ogre::Radian(-ipos.rot[2]), Ogre::Vector3::UNIT_Z);
            const float distance = 50;
            pos = pos + distance*rot.yAxis();
            ipos.pos[0] = pos.x;
            ipos.pos[1] = pos.y;
            ipos.pos[2] = pos.z;
            ipos.rot[0] = 0;
            ipos.rot[1] = 0;
            ipos.rot[2] = 0;

            MWWorld::CellStore* cell = mPlayer->getPlayer().getCell();
            MWWorld::ManualRef ref(getStore(), selectedCreature, 1);
            ref.getPtr().getCellRef().setPosition(ipos);

            safePlaceObject(ref.getPtr(), cell, ipos);
        }
    }

    void World::spawnBloodEffect(const Ptr &ptr, const Vector3 &worldPosition)
    {
        int type = ptr.getClass().getBloodTexture(ptr);
        std::string texture;
        switch (type)
        {
        case 2:
            texture = getFallback()->getFallbackString("Blood_Texture_2");
            break;
        case 1:
            texture = getFallback()->getFallbackString("Blood_Texture_1");
            break;
        case 0:
        default:
            texture = getFallback()->getFallbackString("Blood_Texture_0");
            break;
        }

        std::stringstream modelName;
        modelName << "Blood_Model_";
        int roll = std::rand()/ (static_cast<double> (RAND_MAX) + 1) * 3; // [0, 2]
        modelName << roll;
        std::string model = "meshes\\" + getFallback()->getFallbackString(modelName.str());

        mRendering->spawnEffect(model, texture, worldPosition);
    }

    void World::spawnEffect(const std::string &model, const std::string &textureOverride, const Vector3 &worldPos)
    {
        mRendering->spawnEffect(model, textureOverride, worldPos);
    }

    void World::explodeSpell(const Vector3 &origin, const ESM::EffectList &effects, const Ptr &caster,
                             const std::string& id, const std::string& sourceName)
    {
        std::map<MWWorld::Ptr, std::vector<ESM::ENAMstruct> > toApply;
        for (std::vector<ESM::ENAMstruct>::const_iterator effectIt = effects.mList.begin();
             effectIt != effects.mList.end(); ++effectIt)
        {
            const ESM::MagicEffect* effect = getStore().get<ESM::MagicEffect>().find(effectIt->mEffectID);

            if (effectIt->mArea <= 0)
                continue; // Not an area effect

            // Spawn the explosion orb effect
            const ESM::Static* areaStatic;
            if (!effect->mCasting.empty())
                areaStatic = getStore().get<ESM::Static>().find (effect->mArea);
            else
                areaStatic = getStore().get<ESM::Static>().find ("VFX_DefaultArea");

            mRendering->spawnEffect("meshes\\" + areaStatic->mModel, "", origin, effectIt->mArea);

            // Play explosion sound (make sure to use NoTrack, since we will delete the projectile now)
            static const std::string schools[] = {
                "alteration", "conjuration", "destruction", "illusion", "mysticism", "restoration"
            };
            {
                MWBase::SoundManager *sndMgr = MWBase::Environment::get().getSoundManager();
                if(!effect->mAreaSound.empty())
                    sndMgr->playManualSound3D(origin, effect->mAreaSound, 1.0f, 1.0f, MWBase::SoundManager::Play_TypeSfx, MWBase::SoundManager::Play_NoTrack);
                else
                    sndMgr->playManualSound3D(origin, schools[effect->mData.mSchool]+" area", 1.0f, 1.0f, MWBase::SoundManager::Play_TypeSfx, MWBase::SoundManager::Play_NoTrack);
            }
            // Get the actors in range of the effect
            std::vector<MWWorld::Ptr> objects;
            MWBase::Environment::get().getMechanicsManager()->getObjectsInRange(
                        origin, feetToGameUnits(effectIt->mArea), objects);

            for (std::vector<MWWorld::Ptr>::iterator affected = objects.begin(); affected != objects.end(); ++affected)
                toApply[*affected].push_back(*effectIt);
        }

        // Now apply the appropriate effects to each actor in range
        for (std::map<MWWorld::Ptr, std::vector<ESM::ENAMstruct> >::iterator apply = toApply.begin(); apply != toApply.end(); ++apply)
        {
            MWWorld::Ptr source = caster;
            // Vanilla-compatible behaviour of never applying the spell to the caster
            // (could be changed by mods later)
            if (apply->first == caster)
                continue;

            if (source.isEmpty())
                source = apply->first;

            MWMechanics::CastSpell cast(source, apply->first);
            cast.mHitPosition = origin;
            cast.mId = id;
            cast.mSourceName = sourceName;
            cast.mStack = false;
            ESM::EffectList effects;
            effects.mList = apply->second;
            cast.inflict(apply->first, caster, effects, ESM::RT_Target, false, true);
        }
    }

    void World::activate(const Ptr &object, const Ptr &actor)
    {
        MWScript::InterpreterContext interpreterContext (&object.getRefData().getLocals(), object);
        interpreterContext.activate (object);

        std::string script = object.getClass().getScript (object);

        breakInvisibility(actor);

        if (!script.empty())
        {
            getLocalScripts().setIgnore (object);
            MWBase::Environment::get().getScriptManager()->run (script, interpreterContext);
        }
        if (!interpreterContext.hasActivationBeenHandled())
            interpreterContext.executeActivation(object, actor);
    }
}
