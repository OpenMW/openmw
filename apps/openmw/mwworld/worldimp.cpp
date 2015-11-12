#include "worldimp.hpp"

#if defined(_WIN32) && !defined(__MINGW32__)
#include <boost/tr1/tr1/unordered_map>
#elif defined HAVE_UNORDERED_MAP
#include <unordered_map>
#else
#include <tr1/unordered_map>
#endif

#include <osg/Group>
#include <osg/ComputeBoundsVisitor>
#include <osg/PositionAttitudeTransform>

#include <components/esm/esmreader.hpp>
#include <components/esm/esmwriter.hpp>
#include <components/esm/cellid.hpp>

#include <components/misc/rng.hpp>

#include <components/files/collections.hpp>
#include <components/misc/resourcehelpers.hpp>
#include <components/resource/resourcesystem.hpp>

#include "../mwbase/environment.hpp"
#include "../mwbase/soundmanager.hpp"
#include "../mwbase/mechanicsmanager.hpp"
#include "../mwbase/windowmanager.hpp"
#include "../mwbase/scriptmanager.hpp"

#include "../mwmechanics/creaturestats.hpp"
#include "../mwmechanics/movement.hpp"
#include "../mwmechanics/npcstats.hpp"
#include "../mwmechanics/spellcasting.hpp"
#include "../mwmechanics/levelledlist.hpp"
#include "../mwmechanics/combat.hpp"
#include "../mwmechanics/aiavoiddoor.hpp" //Used to tell actors to avoid doors

#include "../mwrender/animation.hpp"
#include "../mwrender/renderingmanager.hpp"
#include "../mwrender/camera.hpp"
#include "../mwrender/vismask.hpp"

#include "../mwscript/interpretercontext.hpp"
#include "../mwscript/globalscripts.hpp"

#include "../mwclass/door.hpp"

#include "../mwphysics/physicssystem.hpp"
#include "../mwphysics/actor.hpp"
#include "../mwphysics/collisiontype.hpp"

#include "player.hpp"
#include "manualref.hpp"
#include "cellstore.hpp"
#include "cellfunctors.hpp"
#include "containerstore.hpp"
#include "inventorystore.hpp"
#include "actionteleport.hpp"
#include "projectilemanager.hpp"
#include "weather.hpp"

#include "contentloader.hpp"
#include "esmloader.hpp"

namespace
{

// Wraps a value to (-PI, PI]
void wrap(float& rad)
{
    const float pi = static_cast<float>(osg::PI);
    if (rad>0)
        rad = std::fmod(rad+pi, 2.0f*pi)-pi;
    else
        rad = std::fmod(rad-pi, 2.0f*pi)+pi;
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
            mRendering->skySetDate (mGlobalVariables["day"].getInteger(),
                mGlobalVariables["month"].getInteger());

            mRendering->setSkyEnabled(true);
        }
        else
            mRendering->setSkyEnabled(false);
    }

    World::World (
        osgViewer::Viewer* viewer,
        osg::ref_ptr<osg::Group> rootNode,
        Resource::ResourceSystem* resourceSystem,
        const Files::Collections& fileCollections,
        const std::vector<std::string>& contentFiles,
        ToUTF8::Utf8Encoder* encoder, const std::map<std::string,std::string>& fallbackMap,
        int activationDistanceOverride, const std::string& startCell, const std::string& startupScript,
            const std::string& resourcePath)
    : mResourceSystem(resourceSystem), mFallback(fallbackMap), mPlayer (0), mLocalScripts (mStore),
      mSky (true), mCells (mStore, mEsm),
      mGodMode(false), mScriptsEnabled(true), mContentFiles (contentFiles),
      mActivationDistanceOverride (activationDistanceOverride), mStartupScript(startupScript),
      mStartCell (startCell), mTeleportEnabled(true),
      mLevitationEnabled(true), mGoToJail(false), mDaysInPrison(0)
    {
        mPhysics = new MWPhysics::PhysicsSystem(resourceSystem, rootNode);
        mProjectileManager.reset(new ProjectileManager(rootNode, resourceSystem, mPhysics));
        mRendering = new MWRender::RenderingManager(viewer, rootNode, resourceSystem, &mFallback, resourcePath);

        mEsm.resize(contentFiles.size());
        Loading::Listener* listener = MWBase::Environment::get().getWindowManager()->getLoadingScreen();
        listener->loadingOn();

        GameContentLoader gameContentLoader(*listener);
        EsmLoader esmLoader(mStore, mEsm, encoder, *listener);

        gameContentLoader.addLoader(".esm", &esmLoader);
        gameContentLoader.addLoader(".esp", &esmLoader);
        gameContentLoader.addLoader(".omwgame", &esmLoader);
        gameContentLoader.addLoader(".omwaddon", &esmLoader);
        gameContentLoader.addLoader(".project", &esmLoader);

        loadContentFiles(fileCollections, contentFiles, gameContentLoader);

        listener->loadingOff();

        // insert records that may not be present in all versions of MW
        if (mEsm[0].getFormat() == 0)
            ensureNeededRecords();

        mStore.setUp();
        mStore.movePlayerRecord();

        mSwimHeightScale = mStore.get<ESM::GameSetting>().find("fSwimHeightScale")->getFloat();

        mGlobalVariables.fill (mStore);

        mWeatherManager = new MWWorld::WeatherManager(*mRendering, mFallback, mStore);

        mWorldScene = new Scene(*mRendering, mPhysics);
    }

    void World::startNewGame (bool bypass)
    {
        mGoToJail = false;
        mLevitationEnabled = true;
        mTeleportEnabled = true;

        mGodMode = false;
        mScriptsEnabled = true;
        mSky = true;

        // Rebuild player
        setupPlayer();

        renderPlayer();
        mRendering->resetCamera();

        MWBase::Environment::get().getWindowManager()->updatePlayer();

        // we don't want old weather to persist on a new game
        // Note that if reset later, the initial ChangeWeather that the chargen script calls will be lost.
        delete mWeatherManager;
        mWeatherManager = new MWWorld::WeatherManager(*mRendering, mFallback, mStore);

        if (!bypass)
        {
            // set new game mark
            mGlobalVariables["chargenstate"].setInteger (1);
        }
        else
            mGlobalVariables["chargenstate"].setInteger (-1);

        if (bypass && !mStartCell.empty())
        {
            ESM::Position pos;

            if (findExteriorPosition (mStartCell, pos))
            {
                changeToExteriorCell (pos);
                fixPosition(getPlayerPtr());
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
                mWorldScene->changeToExteriorCell(pos, true);
            }
        }

        if (!bypass)
        {
            std::string video = mFallback.getFallbackString("Movies_New_Game");
            if (!video.empty())
                MWBase::Environment::get().getWindowManager()->playVideo(video, true);
        }

        // enable collision
        if (!mPhysics->toggleCollisionMode())
            mPhysics->toggleCollisionMode();

        if (!mStartupScript.empty())
            MWBase::Environment::get().getWindowManager()->executeInConsole(mStartupScript);
    }

    void World::clear()
    {
        mWeatherManager->clear();
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

        mTeleportEnabled = true;
        mLevitationEnabled = true;

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
            +1 // actorId counter
            +1 // levitation/teleport enabled state
            +1; // camera
    }

    int World::countSavedGameCells() const
    {
        return mCells.countSavedGameRecords();
    }

    void World::write (ESM::ESMWriter& writer, Loading::Listener& progress) const
    {
        // Active cells could have a dirty fog of war, sync it to the CellStore first
        for (Scene::CellStoreCollection::const_iterator iter (mWorldScene->getActiveCells().begin());
            iter!=mWorldScene->getActiveCells().end(); ++iter)
        {
            CellStore* cellstore = *iter;
            MWBase::Environment::get().getWindowManager()->writeFog(cellstore);
        }

        MWMechanics::CreatureStats::writeActorIdCounter(writer);

        mStore.write (writer, progress); // dynamic Store must be written (and read) before Cells, so that
                                         // references to custom made records will be recognized
        mCells.write (writer, progress);
        mGlobalVariables.write (writer, progress);
        mPlayer->write (writer, progress);
        mWeatherManager->write (writer, progress);
        mProjectileManager->write (writer, progress);

        writer.startRecord(ESM::REC_ENAB);
        writer.writeHNT("TELE", mTeleportEnabled);
        writer.writeHNT("LEVT", mLevitationEnabled);
        writer.endRecord(ESM::REC_ENAB);

        writer.startRecord(ESM::REC_CAM_);
        writer.writeHNT("FIRS", isFirstPerson());
        writer.endRecord(ESM::REC_CAM_);
    }

    void World::readRecord (ESM::ESMReader& reader, uint32_t type,
        const std::map<int, int>& contentFileMap)
    {
        switch (type)
        {
            case ESM::REC_ACTC:
                MWMechanics::CreatureStats::readActorIdCounter(reader);
                return;
            case ESM::REC_ENAB:
                reader.getHNT(mTeleportEnabled, "TELE");
                reader.getHNT(mLevitationEnabled, "LEVT");
                return;
            default:
                if (!mStore.readRecord (reader, type) &&
                    !mGlobalVariables.readRecord (reader, type) &&
                    !mPlayer->readRecord (reader, type) &&
                    !mWeatherManager->readRecord (reader, type) &&
                    !mCells.readRecord (reader, type, contentFileMap)
                     && !mProjectileManager->readRecord (reader, type)
                        )
                {
                    throw std::runtime_error ("unknown record in saved game");
                }
                break;
        }
    }

    void World::ensureNeededRecords()
    {
        std::map<std::string, ESM::Variant> gmst;
        // Companion (tribunal)
        gmst["sCompanionShare"] = ESM::Variant("Companion Share");
        gmst["sCompanionWarningMessage"] = ESM::Variant("Warning message");
        gmst["sCompanionWarningButtonOne"] = ESM::Variant("Button 1");
        gmst["sCompanionWarningButtonTwo"] = ESM::Variant("Button 2");
        gmst["sCompanionShare"] = ESM::Variant("Companion Share");
        gmst["sProfitValue"] = ESM::Variant("Profit Value");
        gmst["sTeleportDisabled"] = ESM::Variant("Teleport disabled");
        gmst["sLevitateDisabled"] = ESM::Variant("Levitate disabled");

        // Missing in unpatched MW 1.0
        gmst["sDifficulty"] = ESM::Variant("Difficulty");
        gmst["fDifficultyMult"] = ESM::Variant(5.f);
        gmst["sAuto_Run"] = ESM::Variant("Auto Run");
        gmst["sServiceRefusal"] = ESM::Variant("Service Refusal");
        gmst["sNeedOneSkill"] = ESM::Variant("Need one skill");
        gmst["sNeedTwoSkills"] = ESM::Variant("Need two skills");
        gmst["sEasy"] = ESM::Variant("Easy");
        gmst["sHard"] = ESM::Variant("Hard");
        gmst["sDeleteNote"] = ESM::Variant("Delete Note");
        gmst["sEditNote"] = ESM::Variant("Edit Note");
        gmst["sAdmireSuccess"] = ESM::Variant("Admire Success");
        gmst["sAdmireFail"] = ESM::Variant("Admire Fail");
        gmst["sIntimidateSuccess"] = ESM::Variant("Intimidate Success");
        gmst["sIntimidateFail"] = ESM::Variant("Intimidate Fail");
        gmst["sTauntSuccess"] = ESM::Variant("Taunt Success");
        gmst["sTauntFail"] = ESM::Variant("Taunt Fail");
        gmst["sBribeSuccess"] = ESM::Variant("Bribe Success");
        gmst["sBribeFail"] = ESM::Variant("Bribe Fail");
        gmst["fNPCHealthBarTime"] = ESM::Variant(5.f);
        gmst["fNPCHealthBarFade"] = ESM::Variant(1.f);

        // Werewolf (BM)
        gmst["fWereWolfRunMult"] = ESM::Variant(1.f);
        gmst["fWereWolfSilverWeaponDamageMult"] = ESM::Variant(1.f);


        std::map<std::string, ESM::Variant> globals;
        // vanilla Morrowind does not define dayspassed.
        globals["dayspassed"] = ESM::Variant(1); // but the addons start counting at 1 :(
        globals["werewolfclawmult"] = ESM::Variant(25.f);
        globals["pcknownwerewolf"] = ESM::Variant(0);

        // following should exist in all versions of MW, but not necessarily in TCs
        globals["gamehour"] = ESM::Variant(0.f);
        globals["timescale"] = ESM::Variant(30.f);
        globals["day"] = ESM::Variant(1);
        globals["month"] = ESM::Variant(1);
        globals["year"] = ESM::Variant(1);
        globals["pcrace"] = ESM::Variant(0);
        globals["pchascrimegold"] = ESM::Variant(0);
        globals["pchasgolddiscount"] = ESM::Variant(0);
        globals["crimegolddiscount"] = ESM::Variant(0);
        globals["crimegoldturnin"] = ESM::Variant(0);
        globals["pchasturnin"] = ESM::Variant(0);

        for (std::map<std::string, ESM::Variant>::iterator it = gmst.begin(); it != gmst.end(); ++it)
        {
            if (!mStore.get<ESM::GameSetting>().search(it->first))
            {
                ESM::GameSetting setting;
                setting.mId = it->first;
                setting.mValue = it->second;
                mStore.insertStatic(setting);
            }
        }

        for (std::map<std::string, ESM::Variant>::iterator it = globals.begin(); it != globals.end(); ++it)
        {
            if (!mStore.get<ESM::Global>().search(it->first))
            {
                ESM::Global setting;
                setting.mId = it->first;
                setting.mValue = it->second;
                mStore.insertStatic(setting);
            }
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
            setDay(static_cast<int>(value));
        else if (name=="month")
            setMonth(static_cast<int>(value));
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

        for (Scene::CellStoreCollection::const_iterator iter (mWorldScene->getActiveCells().begin());
            iter!=mWorldScene->getActiveCells().end(); ++iter)
        {
            // TODO: caching still doesn't work efficiently here (only works for the one CellStore that the reference is in)
            CellStore* cellstore = *iter;
            Ptr ptr = mCells.getPtr (lowerCaseName, *cellstore, false);

            if (!ptr.isEmpty())
                return ptr;
        }

        if (!activeOnly)
        {
            ret = mCells.getPtr (lowerCaseName);
            if (!ret.isEmpty())
                return ret;
        }

        for (Scene::CellStoreCollection::const_iterator iter (mWorldScene->getActiveCells().begin());
            iter!=mWorldScene->getActiveCells().end(); ++iter)
        {
            CellStore* cellstore = *iter;
            Ptr ptr = cellstore->searchInContainer(lowerCaseName);
            if (!ptr.isEmpty())
                return ptr;
        }

        Ptr ptr = mPlayer->getPlayer().getClass()
            .getContainerStore(mPlayer->getPlayer()).search(lowerCaseName);

        return ptr;
    }

    Ptr World::getPtr (const std::string& name, bool activeOnly)
    {
        Ptr ret = searchPtr(name, activeOnly);
        if (!ret.isEmpty())
            return ret;
        throw std::runtime_error ("unknown ID: " + name);
    }

    Ptr World::searchPtrViaActorId (int actorId)
    {
        // The player is not registered in any CellStore so must be checked manually
        if (actorId == getPlayerPtr().getClass().getCreatureStats(getPlayerPtr()).getActorId())
            return getPlayerPtr();
        // Now search cells
        return mWorldScene->searchPtrViaActorId (actorId);
    }

    struct FindContainerFunctor
    {
        Ptr mContainedPtr;
        Ptr mResult;

        FindContainerFunctor(const Ptr& containedPtr) : mContainedPtr(containedPtr) {}

        bool operator() (Ptr ptr)
        {
            if (mContainedPtr.getContainerStore() == &ptr.getClass().getContainerStore(ptr))
            {
                mResult = ptr;
                return false;
            }

            return true;
        }
    };

    Ptr World::findContainer(const Ptr& ptr)
    {
        if (ptr.isInCell())
            return Ptr();

        Ptr player = getPlayerPtr();
        if (ptr.getContainerStore() == &player.getClass().getContainerStore(player))
            return player;

        const Scene::CellStoreCollection& collection = mWorldScene->getActiveCells();
        for (Scene::CellStoreCollection::const_iterator cellIt = collection.begin(); cellIt != collection.end(); ++cellIt)
        {
            FindContainerFunctor functor(ptr);
            (*cellIt)->forEachContainer(functor);

            if (!functor.mResult.isEmpty())
                return functor.mResult;
        }

        return Ptr();
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
            if (reference == getPlayerPtr())
                throw std::runtime_error("can not disable player object");

            reference.getRefData().disable();

            if(mWorldScene->getActiveCells().find (reference.getCell())!=mWorldScene->getActiveCells().end() && reference.getRefData().getCount())
                mWorldScene->removeObjectFromScene (reference);
        }
    }

    void World::advanceTime (double hours, bool incremental)
    {
        MWBase::Environment::get().getMechanicsManager()->advanceTime(static_cast<float>(hours * 3600));

        mWeatherManager->advanceTime (hours, incremental);

        hours += mGlobalVariables["gamehour"].getFloat();

        setHour (hours);

        int days = static_cast<int>(hours / 24);

        if (days>0)
            mGlobalVariables["dayspassed"].setInteger (
                days + mGlobalVariables["dayspassed"].getInteger());
    }

    void World::setHour (double hour)
    {
        if (hour<0)
            hour = 0;

        int days = static_cast<int>(hour / 24);

        hour = std::fmod (hour, 24);

        mGlobalVariables["gamehour"].setFloat(static_cast<float>(hour));

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

        mRendering->skySetDate(day, month);
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
        mSky = !mSky;
        mRendering->setSkyEnabled(mSky);
        return mSky;
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
        mPhysics->clearQueuedMovement();

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
        mPhysics->clearQueuedMovement();

        if (mCurrentWorldSpace != "sys::default") // FIXME
        {
            // changed worldspace
            mProjectileManager->clear();
            mRendering->notifyWorldSpaceChanged();
        }
        removeContainerScripts(getPlayerPtr());
        mWorldScene->changeToExteriorCell(position, true);
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
            return static_cast<float>(mActivationDistanceOverride);

        return getStore().get<ESM::GameSetting>().find("iMaxActivateDist")->getFloat() * 5 / 4;
    }

    MWWorld::Ptr World::getFacedObject()
    {
        MWWorld::Ptr facedObject;

        if (MWBase::Environment::get().getWindowManager()->isGuiMode() &&
                MWBase::Environment::get().getWindowManager()->isConsoleMode())
            facedObject = getFacedObject(getMaxActivationDistance() * 50, false);
        else
        {
            float telekinesisRangeBonus =
                    mPlayer->getPlayer().getClass().getCreatureStats(mPlayer->getPlayer()).getMagicEffects()
                    .get(ESM::MagicEffect::Telekinesis).getMagnitude();
            telekinesisRangeBonus = feetToGameUnits(telekinesisRangeBonus);

            float activationDistance = getMaxActivationDistance() + telekinesisRangeBonus;

            facedObject = getFacedObject(activationDistance);
        }

        return facedObject;
    }

    osg::Vec3f getActorHeadPosition(const MWWorld::Ptr& actor, MWRender::RenderingManager* rendering)
    {
        osg::Vec3f origin(actor.getRefData().getPosition().asVec3());

        MWRender::Animation* anim = rendering->getAnimation(actor);
        if (anim != NULL)
        {
            const osg::Node* node = anim->getNode("Head");
            if (node == NULL)
                node = anim->getNode("Bip01 Head");
            if (node != NULL)
            {
                osg::MatrixList mats = node->getWorldMatrices();
                if (mats.size())
                    origin = mats[0].getTrans();
            }
        }
        return origin;
    }

    std::pair<MWWorld::Ptr,osg::Vec3f> World::getHitContact(const MWWorld::Ptr &ptr, float distance)
    {
        const ESM::Position &posdata = ptr.getRefData().getPosition();

        osg::Quat rot = osg::Quat(posdata.rot[0], osg::Vec3f(-1,0,0)) * osg::Quat(posdata.rot[2], osg::Vec3f(0,0,-1));
        osg::Vec3f pos = getActorHeadPosition(ptr, mRendering);

        std::pair<MWWorld::Ptr,osg::Vec3f> result = mPhysics->getHitContact(ptr, pos, rot, distance);
        if(result.first.isEmpty())
            return std::make_pair(MWWorld::Ptr(), osg::Vec3f());

        return std::make_pair(result.first, result.second);
    }

    void World::deleteObject (const Ptr& ptr)
    {
        if (!ptr.getRefData().isDeleted() && ptr.getContainerStore() == NULL)
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

    void World::undeleteObject(const Ptr& ptr)
    {
        if (!ptr.getCellRef().hasContentFile())
            return;
        if (ptr.getRefData().isDeleted())
        {
            ptr.getRefData().setCount(1);
            if (mWorldScene->getActiveCells().find(ptr.getCell()) != mWorldScene->getActiveCells().end()
                    && ptr.getRefData().isEnabled())
            {
                mWorldScene->addObjectToScene(ptr);
                std::string script = ptr.getClass().getScript(ptr);
                if (!script.empty())
                    mLocalScripts.add(script, ptr);
                addContainerScripts(ptr, ptr.getCell());
            }
        }
    }

    MWWorld::Ptr World::moveObject(const Ptr &ptr, CellStore* newCell, float x, float y, float z)
    {
        ESM::Position pos = ptr.getRefData().getPosition();

        pos.pos[0] = x;
        pos.pos[1] = y;
        pos.pos[2] = z;

        ptr.getRefData().setPosition(pos);

        osg::Vec3f vec(x, y, z);

        CellStore *currCell = ptr.isInCell() ? ptr.getCell() : NULL; // currCell == NULL should only happen for player, during initial startup
        bool isPlayer = ptr == mPlayer->getPlayer();
        bool haveToMove = isPlayer || (currCell && mWorldScene->isCellActive(*currCell));
        MWWorld::Ptr newPtr = ptr;

        if (currCell != newCell)
        {
            removeContainerScripts(ptr);

            if (isPlayer)
            {
                if (!newCell->isExterior())
                    changeToInteriorCell(Misc::StringUtils::lowerCase(newCell->getCell()->mName), pos);
                else
                {
                    if (mWorldScene->isCellActive(*newCell))
                        mWorldScene->changePlayerCell(newCell, pos, false);
                    else
                        mWorldScene->changeToExteriorCell(pos, false);
                }
                addContainerScripts (getPlayerPtr(), newCell);
                newPtr = getPlayerPtr();
            }
            else
            {
                bool currCellActive = mWorldScene->isCellActive(*currCell);
                bool newCellActive = mWorldScene->isCellActive(*newCell);
                if (!currCellActive && newCellActive)
                {
                    newPtr = ptr.getClass().copyToCell(ptr, *newCell, pos);
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

                    newPtr = ptr.getClass().copyToCell(ptr, *newCell);
                    newPtr.getRefData().setBaseNode(0);
                }
                else if (!currCellActive && !newCellActive)
                    newPtr = ptr.getClass().copyToCell(ptr, *newCell);
                else // both cells active
                {
                    newPtr = ptr.getClass().copyToCell(ptr, *newCell, pos);

                    mRendering->updatePtr(ptr, newPtr);
                    ptr.getRefData().setBaseNode(NULL);
                    MWBase::Environment::get().getSoundManager()->updatePtr (ptr, newPtr);
                    mPhysics->updatePtr(ptr, newPtr);

                    MWBase::MechanicsManager *mechMgr = MWBase::Environment::get().getMechanicsManager();
                    mechMgr->updateCell(ptr, newPtr);

                    std::string script =
                        ptr.getClass().getScript(ptr);
                    if (!script.empty())
                    {
                        mLocalScripts.remove(ptr);
                        removeContainerScripts (ptr);
                        mLocalScripts.add(script, newPtr);
                        addContainerScripts (newPtr, newCell);
                    }
                }
                ptr.getRefData().setCount(0);
            }
        }
        if (haveToMove && newPtr.getRefData().getBaseNode())
        {
            mRendering->moveObject(newPtr, vec);
            mPhysics->updatePosition(newPtr);
        }
        if (isPlayer)
        {
            mWorldScene->playerMoved(vec);
        }
        return newPtr;
    }

    MWWorld::Ptr World::moveObjectImp(const Ptr& ptr, float x, float y, float z)
    {
        CellStore *cell = ptr.getCell();

        if (cell->isExterior()) {
            int cellX, cellY;
            positionToIndex(x, y, cellX, cellY);

            cell = getExterior(cellX, cellY);
        }

        return moveObject(ptr, cell, x, y, z);
    }

    MWWorld::Ptr World::moveObject (const Ptr& ptr, float x, float y, float z)
    {
        return moveObjectImp(ptr, x, y, z);
    }

    void World::scaleObject (const Ptr& ptr, float scale)
    {
        ptr.getCellRef().setScale(scale);

        mWorldScene->updateObjectScale(ptr);
    }

    void World::rotateObjectImp (const Ptr& ptr, const osg::Vec3f& rot, bool adjust)
    {
        const float pi = static_cast<float>(osg::PI);

        ESM::Position pos = ptr.getRefData().getPosition();
        float *objRot = pos.rot;
        if(adjust)
        {
            objRot[0] += rot.x();
            objRot[1] += rot.y();
            objRot[2] += rot.z();
        }
        else
        {
            objRot[0] = rot.x();
            objRot[1] = rot.y();
            objRot[2] = rot.z();
        }

        if(ptr.getClass().isActor())
        {
            /* HACK? Actors shouldn't really be rotating around X (or Y), but
             * currently it's done so for rotating the camera, which needs
             * clamping.
             */
            const float half_pi = pi/2.f;

            if(objRot[0] < -half_pi)     objRot[0] = -half_pi;
            else if(objRot[0] > half_pi) objRot[0] =  half_pi;

            wrap(objRot[1]);
            wrap(objRot[2]);
        }

        ptr.getRefData().setPosition(pos);

        if(ptr.getRefData().getBaseNode() != 0)
            mWorldScene->updateObjectRotation(ptr, true);
    }

    void World::adjustPosition(const Ptr &ptr, bool force)
    {
        ESM::Position pos (ptr.getRefData().getPosition());

        if(!ptr.getRefData().getBaseNode())
        {
            // will be adjusted when Ptr's cell becomes active
            return;
        }

        float terrainHeight = -std::numeric_limits<float>::max();
        if (ptr.getCell()->isExterior())
            terrainHeight = mRendering->getTerrainHeightAt(pos.asVec3());

        if (pos.pos[2] < terrainHeight)
            pos.pos[2] = terrainHeight;

        pos.pos[2] += 20; // place slightly above. will snap down to ground with code below

        ptr.getRefData().setPosition(pos);

        if (force || !isFlying(ptr))
        {
            osg::Vec3f traced = mPhysics->traceDown(ptr, 500);
            if (traced.z() < pos.pos[2])
                pos.pos[2] = traced.z();
        }

        moveObject(ptr, ptr.getCell(), pos.pos[0], pos.pos[1], pos.pos[2]);
    }

    void World::fixPosition(const Ptr &actor)
    {
        const float dist = 8000;
        ESM::Position pos (actor.getRefData().getPosition());
        pos.pos[2] += dist;
        actor.getRefData().setPosition(pos);

        osg::Vec3f traced = mPhysics->traceDown(actor, dist*1.1f);
        moveObject(actor, actor.getCell(), traced.x(), traced.y(), traced.z());
    }

    void World::rotateObject (const Ptr& ptr,float x,float y,float z, bool adjust)
    {
        rotateObjectImp(ptr, osg::Vec3f(x, y, z), adjust);
    }

    MWWorld::Ptr World::safePlaceObject(const MWWorld::Ptr& ptr, MWWorld::CellStore* cell, ESM::Position pos)
    {
        return copyObjectToCell(ptr,cell,pos,false);
    }

    void World::indexToPosition (int cellX, int cellY, float &x, float &y, bool centre) const
    {
        const int cellSize = 8192;

        x = static_cast<float>(cellSize * cellX);
        y = static_cast<float>(cellSize * cellY);

        if (centre)
        {
            x += cellSize/2;
            y += cellSize/2;
        }
    }

    void World::positionToIndex (float x, float y, int &cellX, int &cellY) const
    {
        const int cellSize = 8192;

        cellX = static_cast<int>(std::floor(x / cellSize));
        cellY = static_cast<int>(std::floor(y / cellSize));
    }

    void World::queueMovement(const Ptr &ptr, const osg::Vec3f &velocity)
    {
        mPhysics->queueObjectMovement(ptr, velocity);
    }

    void World::doPhysics(float duration)
    {
        mPhysics->stepSimulation(duration);
        processDoors(duration);

        mProjectileManager->update(duration);

        const MWPhysics::PtrVelocityList &results = mPhysics->applyQueuedMovement(duration);
        MWPhysics::PtrVelocityList::const_iterator player(results.end());
        for(MWPhysics::PtrVelocityList::const_iterator iter(results.begin());iter != results.end();++iter)
        {
            if(iter->first == getPlayerPtr())
            {
                // Handle player last, in case a cell transition occurs
                player = iter;
                continue;
            }
            moveObjectImp(iter->first, iter->second.x(), iter->second.y(), iter->second.z());
        }
        if(player != results.end())
            moveObjectImp(player->first, player->second.x(), player->second.y(), player->second.z());

        mPhysics->debugDraw();
    }

    bool World::castRay (float x1, float y1, float z1, float x2, float y2, float z2)
    {
        osg::Vec3f a(x1,y1,z1);
        osg::Vec3f b(x2,y2,z2);
        MWPhysics::PhysicsSystem::RayResult result = mPhysics->castRay(a, b, MWWorld::Ptr(), MWPhysics::CollisionType_World);
        return result.mHit;
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
                const ESM::Position& objPos = it->first.getRefData().getPosition();
                float oldRot = objPos.rot[2];

                float minRot = it->first.getCellRef().getPosition().rot[2];
                float maxRot = minRot + osg::DegreesToRadians(90.f);

                float diff = duration * osg::DegreesToRadians(90.f);
                float targetRot = std::min(std::max(minRot, oldRot + diff * (it->second == 1 ? 1 : -1)), maxRot);
                rotateObject(it->first, objPos.rot[0], objPos.rot[1], targetRot);
                // the rotation order we want to use
                mWorldScene->updateObjectRotation(it->first, false);

                bool reached = (targetRot == maxRot && it->second) || targetRot == minRot;

                /// \todo should use convexSweepTest here
                std::vector<MWWorld::Ptr> collisions = mPhysics->getCollisions(it->first, MWPhysics::CollisionType_Actor, MWPhysics::CollisionType_Actor);
                for (std::vector<MWWorld::Ptr>::iterator cit = collisions.begin(); cit != collisions.end(); ++cit)
                {
                    MWWorld::Ptr ptr = *cit;
                    if (ptr.getClass().isActor())
                    {
                        // Collided with actor, ask actor to try to avoid door
                        if(ptr != getPlayerPtr() ) {
                            MWMechanics::AiSequence& seq = ptr.getClass().getCreatureStats(ptr).getAiSequence();
                            if(seq.getTypeId() != MWMechanics::AiPackage::TypeIdAvoidDoor) //Only add it once
                                seq.stack(MWMechanics::AiAvoidDoor(it->first),ptr);
                        }

                        // we need to undo the rotation
                        rotateObject(it->first, objPos.rot[0], objPos.rot[1], oldRot);
                        reached = false;
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

    bool World::toggleRenderMode (MWRender::RenderMode mode)
    {
        switch (mode)
        {
            case MWRender::Render_CollisionDebug:
                return mPhysics->toggleDebugRendering();
            default:
                return mRendering->toggleRenderMode(mode);
        }
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

    const ESM::CreatureLevList *World::createOverrideRecord(const ESM::CreatureLevList &record)
    {
        return mStore.overrideRecord(record);
    }

    const ESM::ItemLevList *World::createOverrideRecord(const ESM::ItemLevList &record)
    {
        return mStore.overrideRecord(record);
    }

    const ESM::NPC *World::createRecord(const ESM::NPC &record)
    {
        bool update = false;

        if (Misc::StringUtils::ciEqual(record.mId, "player"))
        {
            const ESM::NPC *player =
                mPlayer->getPlayer().get<ESM::NPC>()->mBase;

            update = record.isMale() != player->isMale() ||
                     !Misc::StringUtils::ciEqual(record.mRace, player->mRace) ||
                     !Misc::StringUtils::ciEqual(record.mHead, player->mHead) ||
                     !Misc::StringUtils::ciEqual(record.mHair, player->mHair);
        }
        const ESM::NPC *ret = mStore.insert(record);
        if (update) {
            renderPlayer();
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

        updateWeather(duration, paused);

        if (!paused)
            doPhysics (duration);

        mWorldScene->update (duration, paused);

        updateWindowManager ();

        updateSoundListener();

        updatePlayer(paused);
    }

    void World::updatePlayer(bool paused)
    {
        MWWorld::Ptr player = getPlayerPtr();

        // TODO: move to MWWorld::Player

        if (player.getCell()->isExterior())
        {
            ESM::Position pos = player.getRefData().getPosition();
            mPlayer->setLastKnownExteriorPosition(pos.asVec3());
        }

        if (player.getClass().getNpcStats(player).isWerewolf())
            MWBase::Environment::get().getWindowManager()->setWerewolfOverlay(mRendering->getCamera()->isFirstPerson());

        // Sink the camera while sneaking
        bool sneaking = player.getClass().getCreatureStats(getPlayerPtr()).getStance(MWMechanics::CreatureStats::Stance_Sneak);
        bool inair = !isOnGround(player);
        bool swimming = isSwimming(player);

        static const float i1stPersonSneakDelta = getStore().get<ESM::GameSetting>().find("i1stPersonSneakDelta")->getFloat();
        if(!paused && sneaking && !(swimming || inair))
            mRendering->getCamera()->setSneakOffset(i1stPersonSneakDelta);
        else
            mRendering->getCamera()->setSneakOffset(0.f);

        int blind = static_cast<int>(player.getClass().getCreatureStats(player).getMagicEffects().get(ESM::MagicEffect::Blind).getMagnitude());
        MWBase::Environment::get().getWindowManager()->setBlindness(std::max(0, std::min(100, blind)));

        int nightEye = static_cast<int>(player.getClass().getCreatureStats(player).getMagicEffects().get(ESM::MagicEffect::NightEye).getMagnitude());
        mRendering->setNightEyeFactor(std::min(1.f, (nightEye/100.f)));

        mRendering->getCamera()->setCameraDistance();
        if(!mRendering->getCamera()->isFirstPerson())
        {
            osg::Vec3f focal, camera;
            mRendering->getCamera()->getPosition(focal, camera);
            float radius = mRendering->getNearClipDistance()*2.5f;
            MWPhysics::PhysicsSystem::RayResult result = mPhysics->castSphere(focal, camera, radius);
            if (result.mHit)
                mRendering->getCamera()->setCameraDistance((result.mHitPos - focal).length() - radius, false, false);
        }

    }

    void World::updateSoundListener()
    {
        const ESM::Position& refpos = getPlayerPtr().getRefData().getPosition();
        osg::Vec3f playerPos = refpos.asVec3();

        playerPos.z() += 1.85f * mPhysics->getHalfExtents(getPlayerPtr()).z();

        osg::Quat playerOrient = osg::Quat(refpos.rot[1], osg::Vec3f(0,-1,0)) *
                osg::Quat(refpos.rot[0], osg::Vec3f(-1,0,0)) *
                osg::Quat(refpos.rot[2], osg::Vec3f(0,0,-1));

        osg::Vec3f forward = playerOrient * osg::Vec3f(0,1,0);
        osg::Vec3f up = playerOrient * osg::Vec3f(0,0,1);

        MWBase::Environment::get().getSoundManager()->setListenerPosDir(playerPos, forward, up);
    }

    void World::updateWindowManager ()
    {
        // inform the GUI about focused object
        MWWorld::Ptr object = getFacedObject ();

        MWBase::Environment::get().getWindowManager()->setFocusObject(object);

        // retrieve object dimensions so we know where to place the floating label
        if (!object.isEmpty ())
        {
            osg::Vec4f screenBounds = mRendering->getScreenBounds(object);

            MWBase::Environment::get().getWindowManager()->setFocusObjectScreenCoords(
                screenBounds.x(), screenBounds.y(), screenBounds.z(), screenBounds.w());
        }
    }

    MWWorld::Ptr World::getFacedObject(float maxDistance, bool ignorePlayer)
    {
        maxDistance += mRendering->getCameraDistance();

        if (MWBase::Environment::get().getWindowManager()->isGuiMode())
        {
            float x, y;
            MWBase::Environment::get().getWindowManager()->getMousePosition(x, y);
            return mRendering->castCameraToViewportRay(x, y, maxDistance, ignorePlayer).mHitObject;
        }
        else
        {
            return mRendering->castCameraToViewportRay(0.5f, 0.5f, maxDistance, ignorePlayer).mHitObject;
        }
    }

    bool World::isCellExterior() const
    {
        const CellStore *currentCell = mWorldScene->getCurrentCell();
        if (currentCell)
        {
            return currentCell->getCell()->isExterior();
        }
        return false;
    }

    bool World::isCellQuasiExterior() const
    {
        const CellStore *currentCell = mWorldScene->getCurrentCell();
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

    osg::Vec2f World::getNorthVector (CellStore* cell)
    {
        MWWorld::CellRefList<ESM::Static>& statics = cell->get<ESM::Static>();
        MWWorld::LiveCellRef<ESM::Static>* ref = statics.find("northmarker");
        if (!ref)
            return osg::Vec2f(0, 1);

        osg::Quat orient (-ref->mData.getPosition().rot[2], osg::Vec3f(0,0,1));
        osg::Vec3f dir = orient * osg::Vec3f(0,1,0);
        osg::Vec2f d (dir.x(), dir.y());
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

                ESM::CellId cellid;
                if (!ref.mRef.getDestCell().empty())
                {
                    cellid.mWorldspace = ref.mRef.getDestCell();
                    cellid.mPaged = false;
                }
                else
                {
                    cellid.mPaged = true;
                    positionToIndex(
                                ref.mRef.getDoorDest().pos[0],
                                ref.mRef.getDoorDest().pos[1],
                                cellid.mIndex.mX,
                                cellid.mIndex.mY);
                }
                newMarker.dest = cellid;

                ESM::Position pos = ref.mData.getPosition ();

                newMarker.x = pos.pos[0];
                newMarker.y = pos.pos[1];
                out.push_back(newMarker);
            }
        }
    }

    void World::setWaterHeight(const float height)
    {
        mPhysics->setWaterHeight(height);
        mRendering->setWaterHeight(height);
    }

    bool World::toggleWater()
    {
        return mRendering->toggleRenderMode(MWRender::Render_Water);
    }

    bool World::toggleWorld()
    {
        return mRendering->toggleRenderMode(MWRender::Render_Scene);
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
        const float maxDist = 200.f;

        MWRender::RenderingManager::RayResult result = mRendering->castCameraToViewportRay(cursorX, cursorY, maxDist, true, true);

        CellStore* cell = getPlayerPtr().getCell();

        ESM::Position pos = getPlayerPtr().getRefData().getPosition();
        if (result.mHit)
        {
            pos.pos[0] = result.mHitPointWorld.x();
            pos.pos[1] = result.mHitPointWorld.y();
            pos.pos[2] = result.mHitPointWorld.z();
        }
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
        const float maxDist = 200.f;
        MWRender::RenderingManager::RayResult result = mRendering->castCameraToViewportRay(cursorX, cursorY, maxDist, true, true);

        if (result.mHit)
        {
            // check if the wanted position is on a flat surface, and not e.g. against a vertical wall
            if (std::acos((result.mHitNormalWorld/result.mHitNormalWorld.length()) * osg::Vec3f(0,0,1)) >= osg::DegreesToRadians(30.f))
                return false;

            return true;
        }
        else
            return false;
    }


    Ptr World::copyObjectToCell(const Ptr &object, CellStore* cell, ESM::Position pos, bool adjustPos)
    {
        if (cell->isExterior())
        {
            int cellX, cellY;
            positionToIndex(pos.pos[0], pos.pos[1], cellX, cellY);
            cell = mCells.getExterior(cellX, cellY);
        }

        MWWorld::Ptr dropped =
            object.getClass().copyToCell(object, *cell, pos);

        // Reset some position values that could be uninitialized if this item came from a container
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

        if (!object.getClass().isActor() && adjustPos && dropped.getRefData().getBaseNode())
        {
            // Adjust position so the location we wanted ends up in the middle of the object bounding box
            osg::ComputeBoundsVisitor computeBounds;
            computeBounds.setTraversalMask(~MWRender::Mask_ParticleSystem);
            dropped.getRefData().getBaseNode()->accept(computeBounds);
            osg::BoundingBox bounds = computeBounds.getBoundingBox();
            if (bounds.valid())
            {
                bounds.set(bounds._min - pos.asVec3(), bounds._max - pos.asVec3());

                osg::Vec3f adjust (
                            (bounds.xMin() + bounds.xMax()) / 2,
                           (bounds.yMin() + bounds.yMax()) / 2,
                           bounds.zMin()
                           );
                pos.pos[0] -= adjust.x();
                pos.pos[1] -= adjust.y();
                pos.pos[2] -= adjust.z();
                moveObject(dropped, pos.pos[0], pos.pos[1], pos.pos[2]);
            }
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

        osg::Vec3f orig = pos.asVec3();
        orig.z() += 20;
        osg::Vec3f dir (0, 0, -1);

        float len = 1000000.0;

        MWRender::RenderingManager::RayResult result = mRendering->castRay(orig, orig+dir*len, true, true);
        if (result.mHit)
            pos.pos[2] = result.mHitPointWorld.z();

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

    bool World::isFlying(const MWWorld::Ptr &ptr) const
    {
        const MWMechanics::CreatureStats &stats = ptr.getClass().getCreatureStats(ptr);

        if(!ptr.getClass().isActor())
            return false;

        if (stats.isDead())
            return false;

        if (ptr.getClass().canFly(ptr))
            return !stats.isParalyzed();

        if(stats.getMagicEffects().get(ESM::MagicEffect::Levitate).getMagnitude() > 0
                && isLevitationEnabled())
            return true;

        const MWPhysics::Actor* actor = mPhysics->getActor(ptr);
        if(!actor || !actor->getCollisionMode())
            return true;

        return false;
    }

    bool World::isSlowFalling(const MWWorld::Ptr &ptr) const
    {
        if(!ptr.getClass().isActor())
            return false;

        const MWMechanics::CreatureStats &stats = ptr.getClass().getCreatureStats(ptr);
        if(stats.getMagicEffects().get(ESM::MagicEffect::SlowFall).getMagnitude() > 0)
            return true;

        return false;
    }

    bool World::isSubmerged(const MWWorld::Ptr &object) const
    {
        return isUnderwater(object, 1.0f/mSwimHeightScale);
    }

    bool World::isSwimming(const MWWorld::Ptr &object) const
    {
        return isUnderwater(object, mSwimHeightScale);
    }

    bool World::isWading(const MWWorld::Ptr &object) const
    {
        const float kneeDeep = 0.25f;
        return isUnderwater(object, kneeDeep);
    }

    bool World::isUnderwater(const MWWorld::Ptr &object, const float heightRatio) const
    {
        osg::Vec3f pos (object.getRefData().getPosition().asVec3());

        pos.z() += heightRatio*2*mPhysics->getRenderingHalfExtents(object).z();

        return isUnderwater(object.getCell(), pos);
    }

    bool World::isUnderwater(const MWWorld::CellStore* cell, const osg::Vec3f &pos) const
    {
        if (!(cell->getCell()->mData.mFlags & ESM::Cell::HasWater)) {
            return false;
        }
        return pos.z() < cell->getWaterLevel();
    }

    bool World::isOnGround(const MWWorld::Ptr &ptr) const
    {
        return mPhysics->isOnGround(ptr);
    }

    void World::togglePOV()
    {
        mRendering->togglePOV();
    }

    bool World::isFirstPerson() const
    {
        return mRendering->getCamera()->isFirstPerson();
    }

    void World::togglePreviewMode(bool enable)
    {
        mRendering->togglePreviewMode(enable);
    }

    bool World::toggleVanityMode(bool enable)
    {
        return mRendering->toggleVanityMode(enable);
    }

    void World::allowVanityMode(bool allow)
    {
        mRendering->allowVanityMode(allow);
    }

    void World::togglePlayerLooking(bool enable)
    {
        mRendering->togglePlayerLooking(enable);
    }

    void World::changeVanityModeScale(float factor)
    {
        mRendering->changeVanityModeScale(factor);
    }

    bool World::vanityRotateCamera(float * rot)
    {
        return mRendering->vanityRotateCamera(rot);
    }

    void World::setCameraDistance(float dist, bool adjust, bool override_)
    {
        mRendering->setCameraDistance(dist, adjust, override_);
    }

    void World::setupPlayer()
    {
        const ESM::NPC *player = mStore.get<ESM::NPC>().find("player");
        if (!mPlayer)
            mPlayer = new MWWorld::Player(player);
        else
        {
            // Remove the old CharacterController
            MWBase::Environment::get().getMechanicsManager()->remove(getPlayerPtr());
            mPhysics->remove(getPlayerPtr());
            mRendering->removePlayer(getPlayerPtr());

            mPlayer->set(player);
        }

        Ptr ptr = mPlayer->getPlayer();
        mRendering->setupPlayer(ptr);
    }

    void World::renderPlayer()
    {
        MWBase::Environment::get().getMechanicsManager()->remove(getPlayerPtr());

        mRendering->renderPlayer(getPlayerPtr());

        scaleObject(getPlayerPtr(), 1.f); // apply race height

        MWBase::Environment::get().getMechanicsManager()->add(getPlayerPtr());
        MWBase::Environment::get().getMechanicsManager()->watchActor(getPlayerPtr());

        std::string model = getPlayerPtr().getClass().getModel(getPlayerPtr());
        model = Misc::ResourceHelpers::correctActorModelPath(model, mResourceSystem->getVFS());
        mPhysics->remove(getPlayerPtr());
        mPhysics->addActor(getPlayerPtr(), model);
    }

    int World::canRest ()
    {
        CellStore *currentCell = mWorldScene->getCurrentCell();

        Ptr player = mPlayer->getPlayer();
        RefData &refdata = player.getRefData();
        osg::Vec3f playerPos(refdata.getPosition().asVec3());

        const MWPhysics::Actor* actor = mPhysics->getActor(player);
        if (!actor)
            throw std::runtime_error("can't find player");

        if((!actor->getOnGround()&&actor->getCollisionMode()) || isUnderwater(currentCell, playerPos) || isWalkingOnWater(player))
            return 2;

        if((currentCell->getCell()->mData.mFlags&ESM::Cell::NoSleep) ||
           player.getClass().getNpcStats(player).isWerewolf())
            return 1;

        return 0;
    }

    MWRender::Animation* World::getAnimation(const MWWorld::Ptr &ptr)
    {
        if (ptr == getPlayerPtr())
            return mRendering->getPlayerAnimation();
        return mRendering->getAnimation(ptr);
    }

    void World::screenshot(osg::Image* image, int w, int h)
    {
        mRendering->screenshot(image, w, h);
    }

    void World::activateDoor(const MWWorld::Ptr& door)
    {
        int state = door.getClass().getDoorState(door);
        switch (state)
        {
        case 0:
            if (door.getRefData().getPosition().rot[2] == door.getCellRef().getPosition().rot[2])
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
        std::vector<MWWorld::Ptr> actors;
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
        std::vector<MWWorld::Ptr> actors;
        mPhysics->getActorsCollidingWith(object, actors);
        return !actors.empty();
    }

    void World::hurtStandingActors(const Ptr &object, float healthPerSecond)
    {
        if (MWBase::Environment::get().getWindowManager()->isGuiMode())
            return;

        std::vector<MWWorld::Ptr> actors;
        mPhysics->getActorsStandingOn(object, actors);
        for (std::vector<MWWorld::Ptr>::iterator it = actors.begin(); it != actors.end(); ++it)
        {
            MWWorld::Ptr actor = *it;
            MWMechanics::CreatureStats& stats = actor.getClass().getCreatureStats(actor);
            if (stats.isDead())
                continue;
            MWMechanics::DynamicStat<float> health = stats.getHealth();
            health.setCurrent(health.getCurrent()-healthPerSecond*MWBase::Environment::get().getFrameDuration());
            stats.setHealth(health);

            if (healthPerSecond > 0.0f)
            {
                if (actor == getPlayerPtr())
                    MWBase::Environment::get().getWindowManager()->activateHitOverlay(false);

                if (!MWBase::Environment::get().getSoundManager()->getSoundPlaying(actor, "Health Damage"))
                    MWBase::Environment::get().getSoundManager()->playSound3D(actor, "Health Damage", 1.0f, 1.0f);
            }
        }
    }

    void World::hurtCollidingActors(const Ptr &object, float healthPerSecond)
    {
        if (MWBase::Environment::get().getWindowManager()->isGuiMode())
            return;

        std::vector<MWWorld::Ptr> actors;
        mPhysics->getActorsCollidingWith(object, actors);
        for (std::vector<MWWorld::Ptr>::iterator it = actors.begin(); it != actors.end(); ++it)
        {
            MWWorld::Ptr actor = *it;
            MWMechanics::CreatureStats& stats = actor.getClass().getCreatureStats(actor);
            if (stats.isDead())
                continue;
            MWMechanics::DynamicStat<float> health = stats.getHealth();
            health.setCurrent(health.getCurrent()-healthPerSecond*MWBase::Environment::get().getFrameDuration());
            stats.setHealth(health);

            if (healthPerSecond > 0.0f)
            {
                if (actor == getPlayerPtr())
                    MWBase::Environment::get().getWindowManager()->activateHitOverlay(false);

                if (!MWBase::Environment::get().getSoundManager()->getSoundPlaying(actor, "Health Damage"))
                    MWBase::Environment::get().getSoundManager()->playSound3D(actor, "Health Damage", 1.0f, 1.0f);
            }
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

    osg::Vec3f World::getStormDirection() const
    {
        if (isCellExterior() || isCellQuasiExterior())
            return mWeatherManager->getStormDirection();
        else
            return osg::Vec3f(0,1,0);
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
                if (ptr.getRefData().isDeleted())
                    continue;
                if (Misc::StringUtils::ciEqual(ptr.getCellRef().getOwner(), npc.getCellRef().getRefId()))
                    out.push_back(ptr);
            }
        }
    }

    struct ListObjectsFunctor
    {
        std::vector<MWWorld::Ptr> mObjects;

        bool operator() (Ptr ptr)
        {
            if (ptr.getRefData().getBaseNode())
                mObjects.push_back(ptr);
            return true;
        }
    };

    void World::getItemsOwnedBy (const MWWorld::Ptr& npc, std::vector<MWWorld::Ptr>& out)
    {
        const Scene::CellStoreCollection& collection = mWorldScene->getActiveCells();
        for (Scene::CellStoreCollection::const_iterator cellIt = collection.begin(); cellIt != collection.end(); ++cellIt)
        {
            ListObjectsFunctor functor;
            (*cellIt)->forEach<ListObjectsFunctor>(functor);

            for (std::vector<MWWorld::Ptr>::iterator it = functor.mObjects.begin(); it != functor.mObjects.end(); ++it)
                if (Misc::StringUtils::ciEqual(it->getCellRef().getOwner(), npc.getCellRef().getRefId()))
                    out.push_back(*it);
        }
    }

    bool World::getLOS(const MWWorld::Ptr& actor, const MWWorld::Ptr& targetActor)
    {
        if (!targetActor.getRefData().isEnabled() || !actor.getRefData().isEnabled())
            return false; // cannot get LOS unless both NPC's are enabled
        if (!targetActor.getRefData().getBaseNode() || !targetActor.getRefData().getBaseNode())
            return false; // not in active cell

        return mPhysics->getLineOfSight(actor, targetActor);
    }

    float World::getDistToNearestRayHit(const osg::Vec3f& from, const osg::Vec3f& dir, float maxDist)
    {
        osg::Vec3f to (dir);
        to.normalize();
        to = from + (to * maxDist);

        MWPhysics::PhysicsSystem::RayResult result = mPhysics->castRay(from, to, MWWorld::Ptr(),
            MWPhysics::CollisionType_World|MWPhysics::CollisionType_HeightMap);

        if (!result.mHit)
            return maxDist;
        else
            return (result.mHitPos - from).length();
    }

    void World::enableActorCollision(const MWWorld::Ptr& actor, bool enable)
    {
        MWPhysics::Actor *physicActor = mPhysics->getActor(actor);
        if (physicActor)
            physicActor->enableCollisionBody(enable);
    }

    bool World::findInteriorPosition(const std::string &name, ESM::Position &pos)
    {
        typedef MWWorld::CellRefList<ESM::Door>::List DoorList;
        typedef MWWorld::CellRefList<ESM::Static>::List StaticList;

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
        // Fall back to the first static location.
        const StaticList &statics = cellStore->get<ESM::Static>().mList;
        if ( statics.begin() != statics.end() ) {
            pos = statics.begin()->mRef.getPosition();
            return true;
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

    void World::reattachPlayerCamera()
    {
        mRendering->rebuildPtr(getPlayerPtr());
    }

    void World::setWerewolf(const MWWorld::Ptr& actor, bool werewolf)
    {
        MWMechanics::NpcStats& npcStats = actor.getClass().getNpcStats(actor);

        // The actor does not have to change state
        if (npcStats.isWerewolf() == werewolf)
            return;

        if (actor == getPlayerPtr())
        {
            if (werewolf)
            {
                mPlayer->saveSkillsAttributes();
                mPlayer->setWerewolfSkillsAttributes();
            }
            else
                mPlayer->restoreSkillsAttributes();
        }

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

        if(actor == getPlayerPtr())
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

            windowManager->setWerewolfOverlay(werewolf);

            // Witnesses of the player's transformation will make them a globally known werewolf
            std::vector<MWWorld::Ptr> closeActors;
            MWBase::Environment::get().getMechanicsManager()->getActorsInRange(actor.getRefData().getPosition().asVec3(),
                                                                               getStore().get<ESM::GameSetting>().search("fAlarmRadius")->getFloat(),
                                                                               closeActors);

            bool detected = false, reported = false;
            for (std::vector<MWWorld::Ptr>::const_iterator it = closeActors.begin(); it != closeActors.end(); ++it)
            {
                if (*it == actor)
                    continue;

                if (!it->getClass().isNpc())
                    continue;

                if (getLOS(*it, actor) && MWBase::Environment::get().getMechanicsManager()->awarenessCheck(actor, *it))
                    detected = true;
                if (it->getClass().getCreatureStats(*it).getAiSetting(MWMechanics::CreatureStats::AI_Alarm).getModified() > 0)
                    reported = true;
            }

            if (detected)
            {
                windowManager->messageBox("#{sWerewolfAlarmMessage}");
                setGlobalInt("pcknownwerewolf", 1);

                if (reported)
                {
                    npcStats.setBounty(npcStats.getBounty()+
                                       mStore.get<ESM::GameSetting>().find("iWereWolfBounty")->getInt());
                    windowManager->messageBox("#{sCrimeMessage}");
                }
            }
        }
    }

    void World::applyWerewolfAcrobatics(const Ptr &actor)
    {
        const Store<ESM::GameSetting> &gmst = getStore().get<ESM::GameSetting>();
        MWMechanics::NpcStats &stats = actor.getClass().getNpcStats(actor);

        stats.getSkill(ESM::Skill::Acrobatics).setBase(gmst.find("fWerewolfAcrobatics")->getInt());
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

    bool World::toggleScripts()
    {
        mScriptsEnabled = !mScriptsEnabled;
        return mScriptsEnabled;
    }

    bool World::getScriptsEnabled() const
    {
        return mScriptsEnabled;
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
            const ESM::Spell* spell = getStore().get<ESM::Spell>().find(selectedSpell);

            // Check mana
            MWMechanics::DynamicStat<float> magicka = stats.getMagicka();
            if (magicka.getCurrent() < spell->mData.mCost && !(isPlayer && getGodModeState()))
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

        // Get the target to use for "on touch" effects, using the facing direction from Head node
        MWWorld::Ptr target;
        float distance = 192.f; // ??
        osg::Vec3f hitPosition = actor.getRefData().getPosition().asVec3();
        osg::Vec3f origin = getActorHeadPosition(actor, mRendering);

        osg::Quat orient = osg::Quat(actor.getRefData().getPosition().rot[0], osg::Vec3f(-1,0,0))
                * osg::Quat(actor.getRefData().getPosition().rot[2], osg::Vec3f(0,0,-1));

        osg::Vec3f direction = orient * osg::Vec3f(0,1,0);
        osg::Vec3f dest = origin + direction * distance;

        // For actor targets, we want to use bounding boxes (physics raycast).
        // This is to give a slight tolerance for errors, especially with creatures like the Skeleton that would be very hard to aim at otherwise.
        // For object targets, we want the detailed shapes (rendering raycast).
        // If we used the bounding boxes for static objects, then we would not be able to target e.g. objects lying on a shelf.

        MWPhysics::PhysicsSystem::RayResult result1 = mPhysics->castRay(origin, dest, actor, MWPhysics::CollisionType_Actor);

        MWRender::RenderingManager::RayResult result2 = mRendering->castRay(origin, dest, true, true);

        float dist1 = std::numeric_limits<float>::max();
        float dist2 = std::numeric_limits<float>::max();

        if (result1.mHit)
            dist1 = (origin - result1.mHitPos).length();
        if (result2.mHit)
            dist2 = (origin - result2.mHitPointWorld).length();

        if (dist1 <= dist2 && result1.mHit)
        {
            target = result1.mHitObject;
            hitPosition = result1.mHitPos;
        }
        else if (result2.mHit)
        {
            target = result2.mHitObject;
            hitPosition = result2.mHitPointWorld;
        }

        std::string selectedSpell = stats.getSpells().getSelectedSpell();

        MWMechanics::CastSpell cast(actor, target);
        cast.mHitPosition = hitPosition;

        if (!selectedSpell.empty())
        {
            const ESM::Spell* spell = getStore().get<ESM::Spell>().find(selectedSpell);

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
                                   const osg::Vec3f& worldPos, const osg::Quat& orient, MWWorld::Ptr bow, float speed, float attackStrength)
    {
        mProjectileManager->launchProjectile(actor, projectile, worldPos, orient, bow, speed, attackStrength);
    }

    void World::launchMagicBolt (const std::string& model, const std::string &sound, const std::string &spellId,
                                 float speed, bool stack, const ESM::EffectList& effects,
                                   const MWWorld::Ptr& caster, const std::string& sourceName, const osg::Vec3f& fallbackDirection)
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

    bool World::findInteriorPositionInWorldSpace(MWWorld::CellStore* cell, osg::Vec3f& result)
    {
        if (cell->isExterior())
            return false;

        // Search for a 'nearest' exterior, counting each cell between the starting
        // cell and the exterior as a distance of 1.  Will fail for isolated interiors.
        std::set< std::string >checkedCells;
        std::set< std::string >currentCells;
        std::set< std::string >nextCells;
        nextCells.insert( cell->getCell()->mName );

        while ( !nextCells.empty() ) {
            currentCells = nextCells;
            nextCells.clear();
            for( std::set< std::string >::const_iterator i = currentCells.begin(); i != currentCells.end(); ++i ) {
                MWWorld::CellStore *next = getInterior( *i );
                if ( !next ) continue;

                const MWWorld::CellRefList<ESM::Door>& doors = next->getReadOnly<ESM::Door>();
                const CellRefList<ESM::Door>::List& refList = doors.mList;

                // Check if any door in the cell leads to an exterior directly
                for (CellRefList<ESM::Door>::List::const_iterator it = refList.begin(); it != refList.end(); ++it)
                {
                    const MWWorld::LiveCellRef<ESM::Door>& ref = *it;
                    if (!ref.mRef.getTeleport()) continue;

                    if (ref.mRef.getDestCell().empty())
                    {
                        ESM::Position pos = ref.mRef.getDoorDest();
                        result = pos.asVec3();
                        return true;
                    }
                    else
                    {
                        std::string dest = ref.mRef.getDestCell();
                        if ( !checkedCells.count(dest) && !currentCells.count(dest) )
                            nextCells.insert(dest);
                    }
                }

                checkedCells.insert( *i );
            }
        }

        // No luck :(
        return false;
    }

    MWWorld::Ptr World::getClosestMarker( const MWWorld::Ptr &ptr, const std::string &id )
    {
        if ( ptr.getCell()->isExterior() ) {
            return getClosestMarkerFromExteriorPosition(mPlayer->getLastKnownExteriorPosition(), id);
        }

        // Search for a 'nearest' marker, counting each cell between the starting
        // cell and the exterior as a distance of 1.  If an exterior is found, jump
        // to the nearest exterior marker, without further interior searching.
        std::set< std::string >checkedCells;
        std::set< std::string >currentCells;
        std::set< std::string >nextCells;
        MWWorld::Ptr closestMarker;

        nextCells.insert( ptr.getCell()->getCell()->mName );
        while ( !nextCells.empty() ) {
            currentCells = nextCells;
            nextCells.clear();
            for( std::set< std::string >::const_iterator i = currentCells.begin(); i != currentCells.end(); ++i ) {
                MWWorld::CellStore *next = getInterior( *i );
                checkedCells.insert( *i );
                if ( !next ) continue;

                closestMarker = next->search( id );
                if ( !closestMarker.isEmpty() )
                {
                    return closestMarker;
                }

                const MWWorld::CellRefList<ESM::Door>& doors = next->getReadOnly<ESM::Door>();
                const CellRefList<ESM::Door>::List& doorList = doors.mList;

                // Check if any door in the cell leads to an exterior directly
                for (CellRefList<ESM::Door>::List::const_iterator it = doorList.begin(); it != doorList.end(); ++it)
                {
                    const MWWorld::LiveCellRef<ESM::Door>& ref = *it;

                    if (!ref.mRef.getTeleport()) continue;

                    if (ref.mRef.getDestCell().empty())
                    {
                        osg::Vec3f worldPos = ref.mRef.getDoorDest().asVec3();
                        return getClosestMarkerFromExteriorPosition(worldPos, id);
                    }
                    else
                    {
                        std::string dest = ref.mRef.getDestCell();
                        if ( !checkedCells.count(dest) && !currentCells.count(dest) )
                            nextCells.insert(dest);
                    }
                }
            }
        }

        return MWWorld::Ptr();
    }

    MWWorld::Ptr World::getClosestMarkerFromExteriorPosition( const osg::Vec3f& worldPos, const std::string &id ) {
        MWWorld::Ptr closestMarker;
        float closestDistance = std::numeric_limits<float>::max();

        std::vector<MWWorld::Ptr> markers;
        mCells.getExteriorPtrs(id, markers);
        for (std::vector<MWWorld::Ptr>::iterator it2 = markers.begin(); it2 != markers.end(); ++it2)
        {
            ESM::Position pos = it2->getRefData().getPosition();
            osg::Vec3f markerPos = pos.asVec3();
            float distance = (worldPos - markerPos).length2();
            if (distance < closestDistance)
            {
                closestDistance = distance;
                closestMarker = *it2;
            }

        }

        return closestMarker;
    }


    void World::teleportToClosestMarker (const MWWorld::Ptr& ptr,
                                          const std::string& id)
    {
        MWWorld::Ptr closestMarker = getClosestMarker( ptr, id );

        if ( closestMarker.isEmpty() )
        {
            std::cerr << "Failed to teleport: no closest marker found" << std::endl;
            return;
        }

        std::string cellName;
        if ( !closestMarker.mCell->isExterior() )
            cellName = closestMarker.mCell->getCell()->mName;

        MWWorld::ActionTeleport action(cellName, closestMarker.getRefData().getPosition(), false);
        action.execute(ptr);
    }

    void World::updateWeather(float duration, bool paused)
    {
        if (mPlayer->wasTeleported())
        {
            mPlayer->setTeleported(false);
            mWeatherManager->playerTeleported();
        }

        mWeatherManager->update(duration, paused);
    }

    struct AddDetectedReference
    {
        AddDetectedReference(std::vector<Ptr>& out, Ptr detector, World::DetectionType type, float squaredDist)
            : mOut(out), mDetector(detector), mSquaredDist(squaredDist), mType(type)
        {
        }

        std::vector<Ptr>& mOut;
        Ptr mDetector;
        float mSquaredDist;
        World::DetectionType mType;
        bool operator() (MWWorld::Ptr ptr)
        {
            if ((ptr.getRefData().getPosition().asVec3() - mDetector.getRefData().getPosition().asVec3()).length2() >= mSquaredDist)
                return true;

            if (!ptr.getRefData().isEnabled() || ptr.getRefData().isDeleted())
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

                if (ptr.getClass().getCreatureStats(ptr).isDead())
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
            dist = effects.get(ESM::MagicEffect::DetectAnimal).getMagnitude();
        else if (type == World::Detect_Key)
            dist = effects.get(ESM::MagicEffect::DetectKey).getMagnitude();
        else if (type == World::Detect_Enchantment)
            dist = effects.get(ESM::MagicEffect::DetectEnchantment).getMagnitude();

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

        int discount = static_cast<int>(bounty * fCrimeGoldDiscountMult);
        int turnIn = static_cast<int>(bounty * fCrimeGoldTurnInMult);

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
        MWWorld::Ptr prisonMarker = getClosestMarker( ptr, "prisonmarker" );
        if ( prisonMarker.isEmpty() )
        {
            std::cerr << "Failed to confiscate items: no closest prison marker found." << std::endl;
            return;
        }
        std::string prisonName = prisonMarker.mRef->mRef.getDestCell();
        if ( prisonName.empty() )
        {
            std::cerr << "Failed to confiscate items: prison marker not linked to prison interior" << std::endl;
            return;
        }
        MWWorld::CellStore *prison = getInterior( prisonName );
        if ( !prison )
        {
            std::cerr << "Failed to confiscate items: failed to load cell " << prisonName << std::endl;
            return;
        }

        MWWorld::Ptr closestChest = prison->search( "stolen_goods" );
        if (!closestChest.isEmpty()) //Found a close chest
        {
            MWBase::Environment::get().getMechanicsManager()->confiscateStolenItems(ptr, closestChest);
        }
        else
            std::cerr << "Failed to confiscate items: no stolen_goods container found" << std::endl;
    }

    void World::goToJail()
    {
        if (!mGoToJail)
        {
            // Reset bounty and forget the crime now, but don't change cell yet (the player should be able to read the dialog text first)
            mGoToJail = true;

            MWWorld::Ptr player = getPlayerPtr();

            int bounty = player.getClass().getNpcStats(player).getBounty();
            player.getClass().getNpcStats(player).setBounty(0);
            mPlayer->recordCrimeId();
            confiscateStolenItems(player);

            int iDaysinPrisonMod = getStore().get<ESM::GameSetting>().find("iDaysinPrisonMod")->getInt();
            mDaysInPrison = std::max(1, bounty / iDaysinPrisonMod);

            return;
        }
        else
        {
            mGoToJail = false;

            MWBase::Environment::get().getWindowManager()->removeGuiMode(MWGui::GM_Dialogue);

            MWBase::Environment::get().getWindowManager()->goToJail(mDaysInPrison);
        }
    }

    void World::spawnRandomCreature(const std::string &creatureList)
    {
        const ESM::CreatureLevList* list = getStore().get<ESM::CreatureLevList>().find(creatureList);

        int iNumberCreatures = getStore().get<ESM::GameSetting>().find("iNumberCreatures")->getInt();
        int numCreatures = 1 + Misc::Rng::rollDice(iNumberCreatures); // [1, iNumberCreatures]

        for (int i=0; i<numCreatures; ++i)
        {
            std::string selectedCreature = MWMechanics::getLevelledItem(list, true);
            if (selectedCreature.empty())
                return;

            ESM::Position ipos = mPlayer->getPlayer().getRefData().getPosition();
            osg::Vec3f pos(ipos.asVec3());
            osg::Quat rot(-ipos.rot[2], osg::Vec3f(0,0,1));
            const float distance = 50;
            pos = pos + (rot * osg::Vec3f(0,1,0)) * distance;
            ipos.pos[0] = pos.x();
            ipos.pos[1] = pos.y();
            ipos.pos[2] = pos.z();
            ipos.rot[0] = 0;
            ipos.rot[1] = 0;
            ipos.rot[2] = 0;

            MWWorld::CellStore* cell = mPlayer->getPlayer().getCell();
            MWWorld::ManualRef ref(getStore(), selectedCreature, 1);
            ref.getPtr().getCellRef().setPosition(ipos);

            safePlaceObject(ref.getPtr(), cell, ipos);
        }
    }

    void World::spawnBloodEffect(const Ptr &ptr, const osg::Vec3f &worldPosition)
    {
        if (ptr == getPlayerPtr() && Settings::Manager::getBool("hit fader", "GUI"))
            return;

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
        int roll = Misc::Rng::rollDice(3); // [0, 2]
        modelName << roll;
        std::string model = "meshes\\" + getFallback()->getFallbackString(modelName.str());

        mRendering->spawnEffect(model, texture, worldPosition);
    }

    void World::spawnEffect(const std::string &model, const std::string &textureOverride, const osg::Vec3f &worldPos)
    {
        mRendering->spawnEffect(model, textureOverride, worldPos);
    }

    void World::explodeSpell(const osg::Vec3f &origin, const ESM::EffectList &effects, const Ptr &caster, ESM::RangeType rangeType,
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
            if (!effect->mArea.empty())
                areaStatic = getStore().get<ESM::Static>().find (effect->mArea);
            else
                areaStatic = getStore().get<ESM::Static>().find ("VFX_DefaultArea");

            mRendering->spawnEffect("meshes\\" + areaStatic->mModel, "", origin, static_cast<float>(effectIt->mArea));

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
                        origin, feetToGameUnits(static_cast<float>(effectIt->mArea)), objects);
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
            cast.inflict(apply->first, caster, effects, rangeType, false, true);
        }
    }

    void World::activate(const Ptr &object, const Ptr &actor)
    {
        MWScript::InterpreterContext interpreterContext (&object.getRefData().getLocals(), object);
        interpreterContext.activate (object);

        std::string script = object.getClass().getScript (object);

        breakInvisibility(actor);

        if (mScriptsEnabled)
        {
            if (!script.empty())
            {
                getLocalScripts().setIgnore (object);
                MWBase::Environment::get().getScriptManager()->run (script, interpreterContext);
            }
            if (!interpreterContext.hasActivationBeenHandled())
                interpreterContext.executeActivation(object, actor);
        }
        else
            interpreterContext.executeActivation(object, actor);
    }

    struct ResetActorsFunctor
    {
        bool operator() (Ptr ptr)
        {
            // Can't reset actors that were moved to a different cell, because we don't know what cell they came from.
            // This could be fixed once we properly track actor cell changes, but may not be desirable behaviour anyhow.
            if (ptr.getClass().isActor() && ptr.getCellRef().hasContentFile())
            {
                const ESM::Position& origPos = ptr.getCellRef().getPosition();
                MWBase::Environment::get().getWorld()->moveObject(ptr, origPos.pos[0], origPos.pos[1], origPos.pos[2]);
                MWBase::Environment::get().getWorld()->rotateObject(ptr, origPos.rot[0], origPos.rot[1], origPos.rot[2]);
                ptr.getClass().adjustPosition(ptr, false);
            }
            return true;
        }
    };
    void World::resetActors()
    {
        for (Scene::CellStoreCollection::const_iterator iter (mWorldScene->getActiveCells().begin());
            iter!=mWorldScene->getActiveCells().end(); ++iter)
        {
            CellStore* cellstore = *iter;
            ResetActorsFunctor functor;
            cellstore->forEach(functor);
        }
    }

    bool World::isWalkingOnWater(const Ptr &actor)
    {
        MWPhysics::Actor* physicActor = mPhysics->getActor(actor);
        if (physicActor && physicActor->isWalkingOnWater())
            return true;
        return false;
    }

    osg::Vec3f World::aimToTarget(const Ptr &actor, const MWWorld::Ptr& target)
    {
        osg::Vec3f weaponPos = getActorHeadPosition(actor, mRendering);
        osg::Vec3f targetPos = mPhysics->getPosition(target);
        return (targetPos - weaponPos);
    }
}
