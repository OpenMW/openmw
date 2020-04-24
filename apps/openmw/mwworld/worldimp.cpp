#include "worldimp.hpp"

#include <osg/Group>
#include <osg/ComputeBoundsVisitor>

#include <BulletCollision/CollisionDispatch/btCollisionWorld.h>
#include <BulletCollision/CollisionShapes/btCompoundShape.h>

#include <components/debug/debuglog.hpp>

#include <components/esm/esmreader.hpp>
#include <components/esm/esmwriter.hpp>
#include <components/esm/cellid.hpp>

#include <components/misc/constants.hpp>
#include <components/misc/resourcehelpers.hpp>
#include <components/misc/rng.hpp>
#include <components/misc/convert.hpp>

#include <components/files/collections.hpp>

#include <components/resource/bulletshape.hpp>
#include <components/resource/resourcesystem.hpp>

#include <components/sceneutil/positionattitudetransform.hpp>

#include <components/detournavigator/debug.hpp>
#include <components/detournavigator/navigatorimpl.hpp>
#include <components/detournavigator/navigatorstub.hpp>
#include <components/detournavigator/recastglobalallocator.hpp>

#include "../mwbase/environment.hpp"
#include "../mwbase/soundmanager.hpp"
#include "../mwbase/mechanicsmanager.hpp"
#include "../mwbase/windowmanager.hpp"
#include "../mwbase/scriptmanager.hpp"

#include "../mwmechanics/creaturestats.hpp"
#include "../mwmechanics/npcstats.hpp"
#include "../mwmechanics/spellcasting.hpp"
#include "../mwmechanics/levelledlist.hpp"
#include "../mwmechanics/combat.hpp"
#include "../mwmechanics/aiavoiddoor.hpp" //Used to tell actors to avoid doors

#include "../mwrender/animation.hpp"
#include "../mwrender/npcanimation.hpp"
#include "../mwrender/renderingmanager.hpp"
#include "../mwrender/camera.hpp"
#include "../mwrender/vismask.hpp"

#include "../mwscript/globalscripts.hpp"

#include "../mwclass/door.hpp"

#include "../mwphysics/physicssystem.hpp"
#include "../mwphysics/actor.hpp"
#include "../mwphysics/collisiontype.hpp"
#include "../mwphysics/object.hpp"
#include "../mwphysics/constants.hpp"

#include "player.hpp"
#include "manualref.hpp"
#include "cellstore.hpp"
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
          typedef std::map<std::string, ContentLoader*> LoadersContainer;
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
            mRendering->skySetDate (mDay->getInteger(), mMonth->getInteger());

            mRendering->setSkyEnabled(true);
        }
        else
            mRendering->setSkyEnabled(false);
    }

    World::World (
        osgViewer::Viewer* viewer,
        osg::ref_ptr<osg::Group> rootNode,
        Resource::ResourceSystem* resourceSystem, SceneUtil::WorkQueue* workQueue,
        const Files::Collections& fileCollections,
        const std::vector<std::string>& contentFiles,
        ToUTF8::Utf8Encoder* encoder, int activationDistanceOverride,
        const std::string& startCell, const std::string& startupScript,
        const std::string& resourcePath, const std::string& userDataPath)
    : mResourceSystem(resourceSystem), mLocalScripts (mStore),
      mSky (true), mCells (mStore, mEsm),
      mGodMode(false), mScriptsEnabled(true), mContentFiles (contentFiles), mUserDataPath(userDataPath),
      mActivationDistanceOverride (activationDistanceOverride),
      mStartCell (startCell), mDistanceToFacedObject(-1), mTeleportEnabled(true),
      mLevitationEnabled(true), mGoToJail(false), mDaysInPrison(0),
      mPlayerTraveling(false), mPlayerInJail(false), mSpellPreloadTimer(0.f)
    {
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

        fillGlobalVariables();

        mStore.setUp(true);
        mStore.movePlayerRecord();

        mSwimHeightScale = mStore.get<ESM::GameSetting>().find("fSwimHeightScale")->mValue.getFloat();

        mPhysics.reset(new MWPhysics::PhysicsSystem(resourceSystem, rootNode));

        if (auto navigatorSettings = DetourNavigator::makeSettingsFromSettingsManager())
        {
            navigatorSettings->mMaxClimb = MWPhysics::sStepSizeUp;
            navigatorSettings->mMaxSlope = MWPhysics::sMaxSlope;
            navigatorSettings->mSwimHeightScale = mSwimHeightScale;
            DetourNavigator::RecastGlobalAllocator::init();
            mNavigator.reset(new DetourNavigator::NavigatorImpl(*navigatorSettings));
        }
        else
        {
            mNavigator.reset(new DetourNavigator::NavigatorStub());
        }

        mRendering.reset(new MWRender::RenderingManager(viewer, rootNode, resourceSystem, workQueue, resourcePath, *mNavigator));
        mProjectileManager.reset(new ProjectileManager(mRendering->getLightRoot(), resourceSystem, mRendering.get(), mPhysics.get()));
        mRendering->preloadCommonAssets();

        mWeatherManager.reset(new MWWorld::WeatherManager(*mRendering, mStore));

        mWorldScene.reset(new Scene(*mRendering.get(), mPhysics.get(), *mNavigator));
    }

    void World::fillGlobalVariables()
    {
        mGlobalVariables.fill (mStore);

        mGameHour = &mGlobalVariables["gamehour"];
        mDaysPassed = &mGlobalVariables["dayspassed"];
        mDay = &mGlobalVariables["day"];
        mMonth = &mGlobalVariables["month"];
        mYear = &mGlobalVariables["year"];
        mTimeScale = &mGlobalVariables["timescale"];
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

        // we don't want old weather to persist on a new game
        // Note that if reset later, the initial ChangeWeather that the chargen script calls will be lost.
        mWeatherManager.reset();
        mWeatherManager.reset(new MWWorld::WeatherManager(*mRendering.get(), mStore));

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
                changeToExteriorCell (pos, true);
                adjustPosition(getPlayerPtr(), false);
            }
            else
            {
                findInteriorPosition (mStartCell, pos);
                changeToInteriorCell (mStartCell, pos, true);
            }
        }
        else
        {
            for (int i=0; i<5; ++i)
                MWBase::Environment::get().getScriptManager()->getGlobalScripts().run();
            if (!getPlayerPtr().isInCell())
            {
                ESM::Position pos;
                const int cellSize = Constants::CellSizeInUnits;
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
            const std::string& video = Fallback::Map::getString("Movies_New_Game");
            if (!video.empty())
                MWBase::Environment::get().getWindowManager()->playVideo(video, true);
        }

        // enable collision
        if (!mPhysics->toggleCollisionMode())
            mPhysics->toggleCollisionMode();

        MWBase::Environment::get().getWindowManager()->updatePlayer();
    }

    void World::clear()
    {
        mWeatherManager->clear();
        mRendering->clear();
        mProjectileManager->clear();
        mLocalScripts.clear();

        mWorldScene->clear();

        mStore.clearDynamic();

        if (mPlayer)
        {
            mPlayer->clear();
            mPlayer->setCell(0);
            mPlayer->getPlayer().getRefData() = RefData();
            mPlayer->set(mStore.get<ESM::NPC>().find ("player"));
        }

        mCells.clear();

        mDoorStates.clear();

        mGoToJail = false;
        mTeleportEnabled = true;
        mLevitationEnabled = true;
        mPlayerTraveling = false;
        mPlayerInJail = false;

        fillGlobalVariables();
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
        for (CellStore* cellstore : mWorldScene->getActiveCells())
        {
            MWBase::Environment::get().getWindowManager()->writeFog(cellstore);
        }

        MWMechanics::CreatureStats::writeActorIdCounter(writer);

        mStore.write (writer, progress); // dynamic Store must be written (and read) before Cells, so that
                                         // references to custom made records will be recognized
        mPlayer->write (writer, progress);
        mCells.write (writer, progress);
        mGlobalVariables.write (writer, progress);
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
            case ESM::REC_PLAY:
                mPlayer->readRecord(reader, type);
                if (getPlayerPtr().isInCell())
                {
                    mWorldScene->preloadCell(getPlayerPtr().getCell(), true);
                    if (getPlayerPtr().getCell()->isExterior())
                        mWorldScene->preloadTerrain(getPlayerPtr().getRefData().getPosition().asVec3());
                }
                break;
            default:
                if (!mStore.readRecord (reader, type) &&
                    !mGlobalVariables.readRecord (reader, type) &&
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
        gmst["fFleeDistance"] = ESM::Variant(3000.f);
        gmst["sMaxSale"] = ESM::Variant("Max Sale");
        gmst["sAnd"] = ESM::Variant("and");

        // Werewolf (BM)
        gmst["fWereWolfRunMult"] = ESM::Variant(1.3f);
        gmst["fWereWolfSilverWeaponDamageMult"] = ESM::Variant(2.f);
        gmst["iWerewolfFightMod"] = ESM::Variant(100);
        gmst["iWereWolfFleeMod"] = ESM::Variant(100);
        gmst["iWereWolfLevelToAttack"] = ESM::Variant(20);
        gmst["iWereWolfBounty"] = ESM::Variant(1000);
        gmst["fCombatDistanceWerewolfMod"] = ESM::Variant(0.3f);

        for (const auto &params : gmst)
        {
            if (!mStore.get<ESM::GameSetting>().search(params.first))
            {
                ESM::GameSetting record;
                record.mId = params.first;
                record.mValue = params.second;
                mStore.insertStatic(record);
            }
        }

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

        for (const auto &params : globals)
        {
            if (!mStore.get<ESM::Global>().search(params.first))
            {
                ESM::Global record;
                record.mId = params.first;
                record.mValue = params.second;
                mStore.insertStatic(record);
            }
        }

        std::map<std::string, std::string> statics;
        // Total conversions from SureAI lack marker records
        statics["divinemarker"] = "marker_divine.nif";
        statics["doormarker"] = "marker_arrow.nif";
        statics["northmarker"] = "marker_north.nif";
        statics["templemarker"] = "marker_temple.nif";
        statics["travelmarker"] = "marker_travel.nif";

        for (const auto &params : statics)
        {
            if (!mStore.get<ESM::Static>().search(params.first))
            {
                ESM::Static record;
                record.mId = params.first;
                record.mModel = params.second;
                mStore.insertStatic(record);
            }
        }

        std::map<std::string, std::string> doors;
        doors["prisonmarker"] = "marker_prison.nif";

        for (const auto &params : doors)
        {
            if (!mStore.get<ESM::Door>().search(params.first))
            {
                ESM::Door record;
                record.mId = params.first;
                record.mModel = params.second;
                mStore.insertStatic(record);
            }
        }
    }

    World::~World()
    {
        // Must be cleared before mRendering is destroyed
        mProjectileManager->clear();
    }

    const ESM::Cell *World::getExterior (const std::string& cellName) const
    {
        // first try named cells
        const ESM::Cell *cell = mStore.get<ESM::Cell>().searchExtByName (cellName);
        if (cell)
            return cell;

        // didn't work -> now check for regions
        for (const ESM::Region &region : mStore.get<ESM::Region>())
        {
            if (Misc::StringUtils::ciEqual(cellName, region.mName))
            {
                return mStore.get<ESM::Cell>().searchExtByRegion(region.mId);
            }
        }

        return nullptr;
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

    void World::testExteriorCells()
    {
        mWorldScene->testExteriorCells();
    }

    void World::testInteriorCells()
    {
        mWorldScene->testInteriorCells();
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

        if (const ESM::Region* region = mStore.get<ESM::Region>().search (cell->getCell()->mRegion))
            return region->mName;

        return mStore.get<ESM::GameSetting>().find ("sDefaultCellname")->mValue.getString();
    }

    void World::removeRefScript (MWWorld::RefData *ref)
    {
        mLocalScripts.remove (ref);
    }

    Ptr World::searchPtr (const std::string& name, bool activeOnly, bool searchInContainers)
    {
        Ptr ret;
        // the player is always in an active cell.
        if (name=="player")
        {
            return mPlayer->getPlayer();
        }

        std::string lowerCaseName = Misc::StringUtils::lowerCase(name);

        for (CellStore* cellstore : mWorldScene->getActiveCells())
        {
            // TODO: caching still doesn't work efficiently here (only works for the one CellStore that the reference is in)
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

        if (searchInContainers)
        {
            for (CellStore* cellstore : mWorldScene->getActiveCells())
            {
                Ptr ptr = cellstore->searchInContainer(lowerCaseName);
                if (!ptr.isEmpty())
                    return ptr;
            }
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

    struct FindContainerVisitor
    {
        ConstPtr mContainedPtr;
        Ptr mResult;

        FindContainerVisitor(const ConstPtr& containedPtr) : mContainedPtr(containedPtr) {}

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

    Ptr World::findContainer(const ConstPtr& ptr)
    {
        if (ptr.isInCell())
            return Ptr();

        Ptr player = getPlayerPtr();
        if (ptr.getContainerStore() == &player.getClass().getContainerStore(player))
            return player;

        for (CellStore* cellstore : mWorldScene->getActiveCells())
        {
            FindContainerVisitor visitor(ptr);
            cellstore->forEachType<ESM::Container>(visitor);
            if (visitor.mResult.isEmpty())
                cellstore->forEachType<ESM::Creature>(visitor);
            if (visitor.mResult.isEmpty())
                cellstore->forEachType<ESM::NPC>(visitor);

            if (!visitor.mResult.isEmpty())
                return visitor.mResult;
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
        if (!incremental)
        {
            // When we fast-forward time, we should recharge magic items
            // in all loaded cells, using game world time
            float duration = hours * 3600;
            const float timeScaleFactor = getTimeScaleFactor();
            if (timeScaleFactor != 0.0f)
                duration /= timeScaleFactor;

            rechargeItems(duration, false);
        }

        mWeatherManager->advanceTime (hours, incremental);

        if (!incremental)
        {
            mRendering->notifyWorldSpaceChanged();
            mProjectileManager->clear();
        }

        hours += mGameHour->getFloat();

        setHour (hours);

        int days = static_cast<int>(hours / 24);

        if (days>0)
            mDaysPassed->setInteger (
                days + mDaysPassed->getInteger());
    }

    void World::setHour (double hour)
    {
        if (hour<0)
            hour = 0;

        int days = static_cast<int>(hour / 24);

        hour = std::fmod (hour, 24);

        mGameHour->setFloat(static_cast<float>(hour));

        if (days>0)
            setDay (days + mDay->getInteger());
    }

    void World::setDay (int day)
    {
        if (day<1)
            day = 1;

        int month = mMonth->getInteger();

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
                mYear->setInteger(mYear->getInteger()+1);
            }

            day -= days;
        }

        mDay->setInteger(day);
        mMonth->setInteger(month);

        mRendering->skySetDate(day, month);
    }

    void World::setMonth (int month)
    {
        if (month<0)
            month = 0;

        int years = month / 12;
        month = month % 12;

        int days = getDaysPerMonth (month);

        if (mDay->getInteger()>days)
            mDay->setInteger (days);

        mMonth->setInteger (month);

        if (years>0)
            mYear->setInteger (years+mYear->getInteger());

        mRendering->skySetDate (mDay->getInteger(), month);
    }

    int World::getDay() const
    {
        return mDay->getInteger();
    }

    int World::getMonth() const
    {
        return mMonth->getInteger();
    }

    int World::getYear() const
    {
        return mYear->getInteger();
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

        return mStore.get<ESM::GameSetting>().find (monthNames[month])->mValue.getString();
    }

    TimeStamp World::getTimeStamp() const
    {
        return TimeStamp (mGameHour->getFloat(), mDaysPassed->getInteger());
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
        return mTimeScale->getFloat();
    }

    void World::changeToInteriorCell (const std::string& cellName, const ESM::Position& position, bool adjustPlayerPos, bool changeEvent)
    {
        mPhysics->clearQueuedMovement();

        if (changeEvent && mCurrentWorldSpace != cellName)
        {
            // changed worldspace
            mProjectileManager->clear();
            mRendering->notifyWorldSpaceChanged();

            mCurrentWorldSpace = cellName;
        }

        removeContainerScripts(getPlayerPtr());
        mWorldScene->changeToInteriorCell(cellName, position, adjustPlayerPos, changeEvent);
        addContainerScripts(getPlayerPtr(), getPlayerPtr().getCell());
    }

    void World::changeToExteriorCell (const ESM::Position& position, bool adjustPlayerPos, bool changeEvent)
    {
        mPhysics->clearQueuedMovement();

        if (changeEvent && mCurrentWorldSpace != ESM::CellId::sDefaultWorldspace)
        {
            // changed worldspace
            mProjectileManager->clear();
            mRendering->notifyWorldSpaceChanged();
        }
        removeContainerScripts(getPlayerPtr());
        mWorldScene->changeToExteriorCell(position, adjustPlayerPos, changeEvent);
        addContainerScripts(getPlayerPtr(), getPlayerPtr().getCell());
    }

    void World::changeToCell (const ESM::CellId& cellId, const ESM::Position& position, bool adjustPlayerPos, bool changeEvent)
    {
        if (!changeEvent)
            mCurrentWorldSpace = cellId.mWorldspace;

        if (cellId.mPaged)
            changeToExteriorCell (position, adjustPlayerPos, changeEvent);
        else
            changeToInteriorCell (cellId.mWorldspace, position, adjustPlayerPos, changeEvent);
    }

    void World::markCellAsUnchanged()
    {
        return mWorldScene->markCellAsUnchanged();
    }

    float World::getMaxActivationDistance ()
    {
        if (mActivationDistanceOverride >= 0)
            return static_cast<float>(mActivationDistanceOverride);

        static const int iMaxActivateDist = mStore.get<ESM::GameSetting>().find("iMaxActivateDist")->mValue.getInteger();
        return static_cast<float>(iMaxActivateDist);
    }

    MWWorld::Ptr World::getFacedObject()
    {
        MWWorld::Ptr facedObject;

        if (MWBase::Environment::get().getWindowManager()->isGuiMode() &&
                MWBase::Environment::get().getWindowManager()->isConsoleMode())
            facedObject = getFacedObject(getMaxActivationDistance() * 50, false);
        else
        {
            float activationDistance = getActivationDistancePlusTelekinesis();

            facedObject = getFacedObject(activationDistance, true);

            if (!facedObject.isEmpty() && !facedObject.getClass().allowTelekinesis(facedObject)
                && mDistanceToFacedObject > getMaxActivationDistance() && !MWBase::Environment::get().getWindowManager()->isGuiMode())
                return 0;
        }
        return facedObject;
    }

   float World::getDistanceToFacedObject()
   {
        return mDistanceToFacedObject;
   }

    osg::Matrixf World::getActorHeadTransform(const MWWorld::ConstPtr& actor) const
    {
        const MWRender::Animation *anim = mRendering->getAnimation(actor);
        if(anim)
        {
            const osg::Node *node = anim->getNode("Head");
            if(!node) node = anim->getNode("Bip01 Head");
            if(node)
            {
                osg::NodePathList nodepaths = node->getParentalNodePaths();
                if(!nodepaths.empty())
                    return osg::computeLocalToWorld(nodepaths[0]);
            }
        }
        return osg::Matrixf::translate(actor.getRefData().getPosition().asVec3());
    }

    std::pair<MWWorld::Ptr,osg::Vec3f> World::getHitContact(const MWWorld::ConstPtr &ptr, float distance, std::vector<MWWorld::Ptr> &targets)
    {
        const ESM::Position &posdata = ptr.getRefData().getPosition();

        osg::Quat rot = osg::Quat(posdata.rot[0], osg::Vec3f(-1,0,0)) * osg::Quat(posdata.rot[2], osg::Vec3f(0,0,-1));

        osg::Vec3f halfExtents = mPhysics->getHalfExtents(ptr);

        // the origin of hitbox is an actor's front, not center
        distance += halfExtents.y();

        // special cased for better aiming with the camera
        // if we do not hit anything, will use the default approach as fallback
        if (ptr == getPlayerPtr())
        {
            osg::Vec3f pos = getActorHeadTransform(ptr).getTrans();

            std::pair<MWWorld::Ptr,osg::Vec3f> result = mPhysics->getHitContact(ptr, pos, rot, distance, targets);
            if(!result.first.isEmpty())
                return std::make_pair(result.first, result.second);
        }

        osg::Vec3f pos = ptr.getRefData().getPosition().asVec3();

        // general case, compatible with all types of different creatures
        // note: we intentionally do *not* use the collision box offset here, this is required to make
        // some flying creatures work that have their collision box offset in the air
        pos.z() += halfExtents.z();

        std::pair<MWWorld::Ptr,osg::Vec3f> result = mPhysics->getHitContact(ptr, pos, rot, distance, targets);
        if(result.first.isEmpty())
            return std::make_pair(MWWorld::Ptr(), osg::Vec3f());

        return std::make_pair(result.first, result.second);
    }

    void World::deleteObject (const Ptr& ptr)
    {
        if (!ptr.getRefData().isDeleted() && ptr.getContainerStore() == nullptr)
        {
            if (ptr == getPlayerPtr())
                throw std::runtime_error("can not delete player object");

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

    MWWorld::Ptr World::moveObject(const Ptr &ptr, CellStore* newCell, float x, float y, float z, bool movePhysics)
    {
        ESM::Position pos = ptr.getRefData().getPosition();

        pos.pos[0] = x;
        pos.pos[1] = y;
        pos.pos[2] = z;

        ptr.getRefData().setPosition(pos);

        osg::Vec3f vec(x, y, z);

        CellStore *currCell = ptr.isInCell() ? ptr.getCell() : nullptr; // currCell == nullptr should only happen for player, during initial startup
        bool isPlayer = ptr == mPlayer->getPlayer();
        bool haveToMove = isPlayer || (currCell && mWorldScene->isCellActive(*currCell));
        MWWorld::Ptr newPtr = ptr;

        if (!isPlayer && !currCell)
           throw std::runtime_error("Can not move actor \"" + ptr.getCellRef().getRefId() + "\" to another cell: current cell is nullptr");

        if (!newCell)
           throw std::runtime_error("Can not move actor \"" + ptr.getCellRef().getRefId() + "\" to another cell: new cell is nullptr");

        if (currCell != newCell)
        {
            removeContainerScripts(ptr);

            if (isPlayer)
            {
                if (!newCell->isExterior())
                {
                    changeToInteriorCell(Misc::StringUtils::lowerCase(newCell->getCell()->mName), pos, false);
                    removeContainerScripts(getPlayerPtr());
                }
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
                    newPtr = currCell->moveTo(ptr, newCell);
                    mWorldScene->addObjectToScene(newPtr);

                    std::string script = newPtr.getClass().getScript(newPtr);
                    if (!script.empty())
                    {
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

                    newPtr = currCell->moveTo(ptr, newCell);
                    newPtr.getRefData().setBaseNode(0);
                }
                else if (!currCellActive && !newCellActive)
                    newPtr = currCell->moveTo(ptr, newCell);
                else // both cells active
                {
                    newPtr = currCell->moveTo(ptr, newCell);

                    mRendering->updatePtr(ptr, newPtr);
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
            }
        }
        if (haveToMove && newPtr.getRefData().getBaseNode())
        {
            mRendering->moveObject(newPtr, vec);
            if (movePhysics)
            {
                mPhysics->updatePosition(newPtr);
                mPhysics->updatePtr(ptr, newPtr);

                if (const auto object = mPhysics->getObject(newPtr))
                    updateNavigatorObject(object);
            }
        }
        if (isPlayer)
        {
            mWorldScene->playerMoved(vec);
        }
        return newPtr;
    }

    MWWorld::Ptr World::moveObjectImp(const Ptr& ptr, float x, float y, float z, bool movePhysics, bool moveToActive)
    {
        int cellX, cellY;
        positionToIndex(x, y, cellX, cellY);

        CellStore* cell = ptr.getCell();
        CellStore* newCell = getExterior(cellX, cellY);
        bool isCellActive = getPlayerPtr().isInCell() && getPlayerPtr().getCell()->isExterior() && mWorldScene->isCellActive(*newCell);

        if (cell->isExterior() || (moveToActive && isCellActive && ptr.getClass().isActor()))
            cell = newCell;

        return moveObject(ptr, cell, x, y, z, movePhysics);
    }

    MWWorld::Ptr World::moveObject (const Ptr& ptr, float x, float y, float z, bool moveToActive)
    {
        return moveObjectImp(ptr, x, y, z, true, moveToActive);
    }

    void World::scaleObject (const Ptr& ptr, float scale)
    {
        if (mPhysics->getActor(ptr))
            mNavigator->removeAgent(getPathfindingHalfExtents(ptr));

        ptr.getCellRef().setScale(scale);

        mWorldScene->updateObjectScale(ptr);

        if (mPhysics->getActor(ptr))
            mNavigator->addAgent(getPathfindingHalfExtents(ptr));
        else if (const auto object = mPhysics->getObject(ptr))
            mShouldUpdateNavigator = updateNavigatorObject(object) || mShouldUpdateNavigator;
    }

    void World::rotateObjectImp(const Ptr& ptr, const osg::Vec3f& rot, MWBase::RotationFlags flags)
    {
        const float pi = static_cast<float>(osg::PI);

        ESM::Position pos = ptr.getRefData().getPosition();
        float *objRot = pos.rot;
        if (flags & MWBase::RotationFlag_adjust)
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
        {
            const auto order = flags & MWBase::RotationFlag_inverseOrder
                ? RotationOrder::inverse : RotationOrder::direct;
            mWorldScene->updateObjectRotation(ptr, order);

            if (const auto object = mPhysics->getObject(ptr))
                updateNavigatorObject(object);
        }
    }

    void World::adjustPosition(const Ptr &ptr, bool force)
    {
        osg::Vec3f pos (ptr.getRefData().getPosition().asVec3());

        if(!ptr.getRefData().getBaseNode())
        {
            // will be adjusted when Ptr's cell becomes active
            return;
        }

        float terrainHeight = -std::numeric_limits<float>::max();
        if (ptr.getCell()->isExterior())
            terrainHeight = getTerrainHeightAt(pos);

        if (pos.z() < terrainHeight)
            pos.z() = terrainHeight;

        pos.z() += 20; // place slightly above. will snap down to ground with code below

        // We still should trace down dead persistent actors - they do not use the "swimdeath" animation.
        bool swims = ptr.getClass().isActor() && isSwimming(ptr) && !(ptr.getClass().isPersistent(ptr) && ptr.getClass().getCreatureStats(ptr).isDeathAnimationFinished());
        if (force || !ptr.getClass().isActor() || (!isFlying(ptr) && !swims && isActorCollisionEnabled(ptr)))
        {
            osg::Vec3f traced = mPhysics->traceDown(ptr, pos, Constants::CellSizeInUnits);
            if (traced.z() < pos.z())
                pos.z() = traced.z();
        }

        moveObject(ptr, ptr.getCell(), pos.x(), pos.y(), pos.z());
    }

    void World::fixPosition()
    {
        const MWWorld::Ptr actor = getPlayerPtr();
        const float distance = 128.f;
        ESM::Position esmPos = actor.getRefData().getPosition();
        osg::Quat orientation(esmPos.rot[2], osg::Vec3f(0,0,-1));
        osg::Vec3f pos (esmPos.asVec3());

        int direction = 0;
        int fallbackDirections[4] = {direction, (direction+3)%4, (direction+2)%4, (direction+1)%4};

        osg::Vec3f targetPos = pos;
        for (int i=0; i<4; ++i)
        {
            direction = fallbackDirections[i];
            if (direction == 0) targetPos = pos + (orientation * osg::Vec3f(0,1,0)) * distance;
            else if(direction == 1) targetPos = pos - (orientation * osg::Vec3f(0,1,0)) * distance;
            else if(direction == 2) targetPos = pos - (orientation * osg::Vec3f(1,0,0)) * distance;
            else if(direction == 3) targetPos = pos + (orientation * osg::Vec3f(1,0,0)) * distance;

            // destination is free
            if (!castRay(pos.x(), pos.y(), pos.z(), targetPos.x(), targetPos.y(), targetPos.z()))
                break;
        }

        targetPos.z() += distance / 2.f; // move up a bit to get out from geometry, will snap down later
        osg::Vec3f traced = mPhysics->traceDown(actor, targetPos, Constants::CellSizeInUnits);
        if (traced != pos)
        {
            esmPos.pos[0] = traced.x();
            esmPos.pos[1] = traced.y();
            esmPos.pos[2] = traced.z();
            MWWorld::ActionTeleport(actor.getCell()->isExterior() ? "" : actor.getCell()->getCell()->mName, esmPos, false).execute(actor);
        }
    }

    void World::rotateObject (const Ptr& ptr, float x, float y, float z, MWBase::RotationFlags flags)
    {
        rotateObjectImp(ptr, osg::Vec3f(x, y, z), flags);
    }

    void World::rotateWorldObject (const Ptr& ptr, osg::Quat rotate)
    {
        if(ptr.getRefData().getBaseNode() != 0)
        {
            mRendering->rotateObject(ptr, rotate);
            mPhysics->updateRotation(ptr);

            if (const auto object = mPhysics->getObject(ptr))
                updateNavigatorObject(object);
        }
    }

    MWWorld::Ptr World::placeObject(const MWWorld::ConstPtr& ptr, MWWorld::CellStore* cell, ESM::Position pos)
    {
        return copyObjectToCell(ptr,cell,pos,ptr.getRefData().getCount(),false);
    }

    MWWorld::Ptr World::safePlaceObject(const ConstPtr &ptr, const ConstPtr &referenceObject, MWWorld::CellStore* referenceCell, int direction, float distance)
    {
        ESM::Position ipos = referenceObject.getRefData().getPosition();
        osg::Vec3f pos(ipos.asVec3());
        osg::Quat orientation(ipos.rot[2], osg::Vec3f(0,0,-1));

        int fallbackDirections[4] = {direction, (direction+3)%4, (direction+2)%4, (direction+1)%4};

        osg::Vec3f spawnPoint = pos;

        for (int i=0; i<4; ++i)
        {
            direction = fallbackDirections[i];
            if (direction == 0) spawnPoint = pos + (orientation * osg::Vec3f(0,1,0)) * distance;
            else if(direction == 1) spawnPoint = pos - (orientation * osg::Vec3f(0,1,0)) * distance;
            else if(direction == 2) spawnPoint = pos - (orientation * osg::Vec3f(1,0,0)) * distance;
            else if(direction == 3) spawnPoint = pos + (orientation * osg::Vec3f(1,0,0)) * distance;

            if (!ptr.getClass().isActor())
                break;

            // check if spawn point is safe, fall back to another direction if not
            spawnPoint.z() += 30; // move up a little to account for slopes, will snap down later

            if (!castRay(spawnPoint.x(), spawnPoint.y(), spawnPoint.z(),
                                                               pos.x(), pos.y(), pos.z() + 20))
            {
                // safe
                break;
            }
        }
        ipos.pos[0] = spawnPoint.x();
        ipos.pos[1] = spawnPoint.y();
        ipos.pos[2] = spawnPoint.z();

        if (!referenceObject.getClass().isActor())
        {
            ipos.rot[0] = referenceObject.getRefData().getPosition().rot[0];
            ipos.rot[1] = referenceObject.getRefData().getPosition().rot[1];
        }
        else
        {
            ipos.rot[0] = 0;
            ipos.rot[1] = 0;
        }
        ipos.rot[2] = referenceObject.getRefData().getPosition().rot[2];

        MWWorld::Ptr placed = copyObjectToCell(ptr, referenceCell, ipos, ptr.getRefData().getCount(), false);
        adjustPosition(placed, true); // snap to ground
        return placed;
    }

    void World::indexToPosition (int cellX, int cellY, float &x, float &y, bool centre) const
    {
        const int cellSize = Constants::CellSizeInUnits;

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
        cellX = static_cast<int>(std::floor(x / Constants::CellSizeInUnits));
        cellY = static_cast<int>(std::floor(y / Constants::CellSizeInUnits));
    }

    void World::queueMovement(const Ptr &ptr, const osg::Vec3f &velocity)
    {
        mPhysics->queueObjectMovement(ptr, velocity);
    }

    void World::updateAnimatedCollisionShape(const Ptr &ptr)
    {
        mPhysics->updateAnimatedCollisionShape(ptr);
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
            moveObjectImp(iter->first, iter->second.x(), iter->second.y(), iter->second.z(), false);
        }
        if(player != results.end())
            moveObjectImp(player->first, player->second.x(), player->second.y(), player->second.z(), false);
    }

    void World::updateNavigator()
    {
        mPhysics->forEachAnimatedObject([&] (const MWPhysics::Object* object)
        {
            mShouldUpdateNavigator = updateNavigatorObject(object) || mShouldUpdateNavigator;
        });

        for (const auto& door : mDoorStates)
            if (const auto object = mPhysics->getObject(door.first))
                mShouldUpdateNavigator = updateNavigatorObject(object) || mShouldUpdateNavigator;

        if (mShouldUpdateNavigator)
        {
            mNavigator->update(getPlayerPtr().getRefData().getPosition().asVec3());
            mShouldUpdateNavigator = false;
        }
    }

    bool World::updateNavigatorObject(const MWPhysics::Object* object)
    {
        const DetourNavigator::ObjectShapes shapes {
            *object->getShapeInstance()->getCollisionShape(),
            object->getShapeInstance()->getAvoidCollisionShape()
        };
        return mNavigator->updateObject(DetourNavigator::ObjectId(object), shapes, object->getCollisionObject()->getWorldTransform());
    }

    bool World::castRay (float x1, float y1, float z1, float x2, float y2, float z2)
    {
        int mask = MWPhysics::CollisionType_World | MWPhysics::CollisionType_Door;
        bool result = castRay(x1, y1, z1, x2, y2, z2, mask);
        return result;
    }

    bool World::castRay (float x1, float y1, float z1, float x2, float y2, float z2, int mask)
    {
        osg::Vec3f a(x1,y1,z1);
        osg::Vec3f b(x2,y2,z2);

        MWPhysics::PhysicsSystem::RayResult result = mPhysics->castRay(a, b, MWWorld::Ptr(), std::vector<MWWorld::Ptr>(), mask);
        return result.mHit;
    }

    bool World::castRay(const osg::Vec3f& from, const osg::Vec3f& to, int mask, const MWWorld::ConstPtr& ignore)
    {
        return mPhysics->castRay(from, to, ignore, std::vector<MWWorld::Ptr>(), mask).mHit;
    }

    bool World::rotateDoor(const Ptr door, MWWorld::DoorState state, float duration)
    {
        const ESM::Position& objPos = door.getRefData().getPosition();
        float oldRot = objPos.rot[2];

        float minRot = door.getCellRef().getPosition().rot[2];
        float maxRot = minRot + osg::DegreesToRadians(90.f);

        float diff = duration * osg::DegreesToRadians(90.f);
        float targetRot = std::min(std::max(minRot, oldRot + diff * (state == MWWorld::DoorState::Opening ? 1 : -1)), maxRot);
        rotateObject(door, objPos.rot[0], objPos.rot[1], targetRot, MWBase::RotationFlag_none);

        bool reached = (targetRot == maxRot && state != MWWorld::DoorState::Idle) || targetRot == minRot;

        /// \todo should use convexSweepTest here
        bool collisionWithActor = false;
        std::vector<MWWorld::Ptr> collisions = mPhysics->getCollisions(door, MWPhysics::CollisionType_Door, MWPhysics::CollisionType_Actor);
        for (MWWorld::Ptr& ptr : collisions)
        {
            if (ptr.getClass().isActor())
            {
                collisionWithActor = true;
                
                // Collided with actor, ask actor to try to avoid door
                if(ptr != getPlayerPtr() )
                {
                    MWMechanics::AiSequence& seq = ptr.getClass().getCreatureStats(ptr).getAiSequence();
                    if(seq.getTypeId() != MWMechanics::AiPackage::TypeIdAvoidDoor) //Only add it once
                        seq.stack(MWMechanics::AiAvoidDoor(door),ptr);
                }

                // we need to undo the rotation
                reached = false;
            }
        }

        // Cancel door closing sound if collision with actor is detected
        if (collisionWithActor)
        {
            const ESM::Door* ref = door.get<ESM::Door>()->mBase;

            if (state == MWWorld::DoorState::Opening)
            {
                const std::string& openSound = ref->mOpenSound;
                if (!openSound.empty() && MWBase::Environment::get().getSoundManager()->getSoundPlaying(door, openSound))
                    MWBase::Environment::get().getSoundManager()->stopSound3D(door, openSound);
            }
            else if (state == MWWorld::DoorState::Closing)
            {
                const std::string& closeSound = ref->mCloseSound;
                if (!closeSound.empty() && MWBase::Environment::get().getSoundManager()->getSoundPlaying(door, closeSound))
                    MWBase::Environment::get().getSoundManager()->stopSound3D(door, closeSound);
            }

            rotateObject(door, objPos.rot[0], objPos.rot[1], oldRot, MWBase::RotationFlag_none);
        }

        return reached;
    }

    void World::processDoors(float duration)
    {
        auto it = mDoorStates.begin();
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
                bool reached = rotateDoor(it->first, it->second, duration);

                if (reached)
                {
                    // Mark as non-moving
                    it->first.getClass().setDoorState(it->first, MWWorld::DoorState::Idle);
                    mDoorStates.erase(it++);
                }
                else
                    ++it;
            }
        }
    }

    void World::setActorCollisionMode(const MWWorld::Ptr& ptr, bool internal, bool external)
    {
        MWPhysics::Actor *physicActor = mPhysics->getActor(ptr);
        if (physicActor && physicActor->getCollisionMode() != internal)
        {
            physicActor->enableCollisionMode(internal);
            physicActor->enableCollisionBody(external);
        }
    }

    bool World::isActorCollisionEnabled(const MWWorld::Ptr& ptr)
    {
        MWPhysics::Actor *physicActor = mPhysics->getActor(ptr);
        return physicActor && physicActor->getCollisionMode();
    }

    bool World::toggleCollisionMode()
    {
        if (mPhysics->toggleCollisionMode())
        {
            adjustPosition(getPlayerPtr(), true);
            return true;
        }

        return false;
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

        // Reset "traveling" flag - there was a frame to detect traveling.
        mPlayerTraveling = false;

        // The same thing for "in jail" flag: reset it if:
        // 1. Player was in jail
        // 2. Jailing window was closed
        if (mPlayerInJail && !mGoToJail && !MWBase::Environment::get().getWindowManager()->containsMode(MWGui::GM_Jail))
            mPlayerInJail = false;

        updateWeather(duration, paused);

        if (!paused)
        {
            updateNavigator();
        }

        updatePlayer();

        mPhysics->debugDraw();

        mWorldScene->update (duration, paused);

        updateSoundListener();

        mSpellPreloadTimer -= duration;
        if (mSpellPreloadTimer <= 0.f)
        {
            mSpellPreloadTimer = 0.1f;
            preloadSpells();
        }
    }

    void World::updatePhysics (float duration, bool paused)
    {
        if (!paused)
        {
            doPhysics (duration);
        }
    }

    void World::updatePlayer()
    {
        MWWorld::Ptr player = getPlayerPtr();

        // TODO: move to MWWorld::Player

        if (player.getCell()->isExterior())
        {
            ESM::Position pos = player.getRefData().getPosition();
            mPlayer->setLastKnownExteriorPosition(pos.asVec3());
        }

        bool isWerewolf = player.getClass().getNpcStats(player).isWerewolf();
        bool isFirstPerson = mRendering->getCamera()->isFirstPerson();
        if (isWerewolf && isFirstPerson)
        {
            float werewolfFov = Fallback::Map::getFloat("General_Werewolf_FOV");
            if (werewolfFov != 0)
                mRendering->overrideFieldOfView(werewolfFov);
            MWBase::Environment::get().getWindowManager()->setWerewolfOverlay(true);
        }
        else
        {
            mRendering->resetFieldOfView();
            MWBase::Environment::get().getWindowManager()->setWerewolfOverlay(false);
        }

        // Sink the camera while sneaking
        bool sneaking = player.getClass().getCreatureStats(getPlayerPtr()).getStance(MWMechanics::CreatureStats::Stance_Sneak);
        bool swimming = isSwimming(player);
        bool flying = isFlying(player);

        static const float i1stPersonSneakDelta = mStore.get<ESM::GameSetting>().find("i1stPersonSneakDelta")->mValue.getFloat();
        if (sneaking && !swimming && !flying)
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

    void World::preloadSpells()
    {
        std::string selectedSpell = MWBase::Environment::get().getWindowManager()->getSelectedSpell();
        if (!selectedSpell.empty())
        {
            const ESM::Spell* spell = mStore.get<ESM::Spell>().search(selectedSpell);
            if (spell)
                preloadEffects(&spell->mEffects);
        }
        const MWWorld::Ptr& selectedEnchantItem = MWBase::Environment::get().getWindowManager()->getSelectedEnchantItem();
        if (!selectedEnchantItem.isEmpty())
        {
            std::string enchantId = selectedEnchantItem.getClass().getEnchantment(selectedEnchantItem);
            if (!enchantId.empty())
            {
                const ESM::Enchantment* ench = mStore.get<ESM::Enchantment>().search(selectedEnchantItem.getClass().getEnchantment(selectedEnchantItem));
                if (ench)
                    preloadEffects(&ench->mEffects);
            }
        }
        const MWWorld::Ptr& selectedWeapon = MWBase::Environment::get().getWindowManager()->getSelectedWeapon();
        if (!selectedWeapon.isEmpty())
        {
            std::string enchantId = selectedWeapon.getClass().getEnchantment(selectedWeapon);
            if (!enchantId.empty())
            {
                const ESM::Enchantment* ench = mStore.get<ESM::Enchantment>().search(enchantId);
                if (ench && ench->mData.mType == ESM::Enchantment::WhenStrikes)
                    preloadEffects(&ench->mEffects);
            }
        }
    }

    void World::updateSoundListener()
    {
        const ESM::Position& refpos = getPlayerPtr().getRefData().getPosition();
        osg::Vec3f listenerPos;

        if (isFirstPerson())
            listenerPos = mRendering->getCameraPosition();
        else
            listenerPos = refpos.asVec3() + osg::Vec3f(0, 0, 1.85f * mPhysics->getHalfExtents(getPlayerPtr()).z());

        osg::Quat listenerOrient = osg::Quat(refpos.rot[1], osg::Vec3f(0,-1,0)) *
                osg::Quat(refpos.rot[0], osg::Vec3f(-1,0,0)) *
                osg::Quat(refpos.rot[2], osg::Vec3f(0,0,-1));

        osg::Vec3f forward = listenerOrient * osg::Vec3f(0,1,0);
        osg::Vec3f up = listenerOrient * osg::Vec3f(0,0,1);

        bool underwater = isUnderwater(getPlayerPtr().getCell(), mRendering->getCameraPosition());

        MWBase::Environment::get().getSoundManager()->setListenerPosDir(listenerPos, forward, up, underwater);
    }

    void World::updateWindowManager ()
    {
        try
        {
            // inform the GUI about focused object
            MWWorld::Ptr object = getFacedObject ();

            // retrieve object dimensions so we know where to place the floating label
            if (!object.isEmpty ())
            {
                osg::Vec4f screenBounds = mRendering->getScreenBounds(object);

                MWBase::Environment::get().getWindowManager()->setFocusObjectScreenCoords(
                    screenBounds.x(), screenBounds.y(), screenBounds.z(), screenBounds.w());
            }

            MWBase::Environment::get().getWindowManager()->setFocusObject(object);
        }
        catch (std::exception& e)
        {
            Log(Debug::Error) << "Error updating window manager: " << e.what();
        }
    }

    MWWorld::Ptr World::getFacedObject(float maxDistance, bool ignorePlayer)
    {
        const float camDist = mRendering->getCameraDistance();
        maxDistance += camDist;
        MWWorld::Ptr facedObject;
        MWRender::RenderingManager::RayResult rayToObject;

        if (MWBase::Environment::get().getWindowManager()->isGuiMode())
        {
            float x, y;
            MWBase::Environment::get().getWindowManager()->getMousePosition(x, y);
            rayToObject = mRendering->castCameraToViewportRay(x, y, maxDistance, ignorePlayer);
        }
        else
            rayToObject = mRendering->castCameraToViewportRay(0.5f, 0.5f, maxDistance, ignorePlayer);

        facedObject = rayToObject.mHitObject;
        if (rayToObject.mHit)
            mDistanceToFacedObject = (rayToObject.mRatio * maxDistance) - camDist;
        else
            mDistanceToFacedObject = -1;
        return facedObject;
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

    unsigned int World::getNightDayMode() const
    {
        return mWeatherManager->getNightDayMode();
    }

    void World::changeWeather(const std::string& region, const unsigned int id)
    {
        mWeatherManager->changeWeather(region, id);
    }

    void World::modRegion(const std::string &regionid, const std::vector<char> &chances)
    {
        mWeatherManager->modRegion(regionid, chances);
    }

    osg::Vec2f World::getNorthVector (const CellStore* cell)
    {
        MWWorld::ConstPtr northmarker = cell->searchConst("northmarker");

        if (northmarker.isEmpty())
            return osg::Vec2f(0, 1);

        osg::Quat orient (-northmarker.getRefData().getPosition().rot[2], osg::Vec3f(0,0,1));
        osg::Vec3f dir = orient * osg::Vec3f(0,1,0);
        osg::Vec2f d (dir.x(), dir.y());
        return d;
    }

    struct GetDoorMarkerVisitor
    {
        GetDoorMarkerVisitor(std::vector<World::DoorMarker>& out)
            : mOut(out)
        {
        }

        std::vector<World::DoorMarker>& mOut;

        bool operator()(const MWWorld::Ptr& ptr)
        {
            MWWorld::LiveCellRef<ESM::Door>& ref = *static_cast<MWWorld::LiveCellRef<ESM::Door>* >(ptr.getBase());

            if (!ref.mData.isEnabled() || ref.mData.isDeleted())
                return true;

            if (ref.mRef.getTeleport())
            {
                World::DoorMarker newMarker;
                newMarker.name = MWClass::Door::getDestination(ref);

                ESM::CellId cellid;
                if (!ref.mRef.getDestCell().empty())
                {
                    cellid.mWorldspace = ref.mRef.getDestCell();
                    cellid.mPaged = false;
                    cellid.mIndex.mX = 0;
                    cellid.mIndex.mY = 0;
                }
                else
                {
                    cellid.mPaged = true;
                    MWBase::Environment::get().getWorld()->positionToIndex(
                                ref.mRef.getDoorDest().pos[0],
                                ref.mRef.getDoorDest().pos[1],
                                cellid.mIndex.mX,
                                cellid.mIndex.mY);
                }
                newMarker.dest = cellid;

                ESM::Position pos = ref.mData.getPosition ();

                newMarker.x = pos.pos[0];
                newMarker.y = pos.pos[1];
                mOut.push_back(newMarker);
            }
            return true;
        }
    };

    void World::getDoorMarkers (CellStore* cell, std::vector<World::DoorMarker>& out)
    {
        GetDoorMarkerVisitor visitor(out);
        cell->forEachType<ESM::Door>(visitor);
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

    bool World::toggleBorders()
    {
        return mRendering->toggleBorders();
    }

    void World::PCDropped (const Ptr& item)
    {
        std::string script = item.getClass().getScript(item);

        // Set OnPCDrop Variable on item's script, if it has a script with that variable declared
        if(script != "")
            item.getRefData().getLocals().setVarByInt(script, "onpcdrop", 1);
    }

    MWWorld::Ptr World::placeObject (const MWWorld::ConstPtr& object, float cursorX, float cursorY, int amount)
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
        Ptr dropped = copyObjectToCell(object, cell, pos, amount, true);

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


    Ptr World::copyObjectToCell(const ConstPtr &object, CellStore* cell, ESM::Position pos, int count, bool adjustPos)
    {
        if (cell->isExterior())
        {
            int cellX, cellY;
            positionToIndex(pos.pos[0], pos.pos[1], cellX, cellY);
            cell = mCells.getExterior(cellX, cellY);
        }

        MWWorld::Ptr dropped =
            object.getClass().copyToCell(object, *cell, pos, count);

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

    MWWorld::Ptr World::dropObjectOnGround (const Ptr& actor, const ConstPtr& object, int amount)
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
        Ptr dropped = copyObjectToCell(object, cell, pos, amount, true);

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
        if(!ptr.getClass().isActor())
            return false;

        const MWMechanics::CreatureStats &stats = ptr.getClass().getCreatureStats(ptr);

        if (stats.isDead())
            return false;

        if (ptr.getClass().canFly(ptr))
            return !stats.isParalyzed();

        if(stats.getMagicEffects().get(ESM::MagicEffect::Levitate).getMagnitude() > 0
                && isLevitationEnabled())
            return true;

        const MWPhysics::Actor* actor = mPhysics->getActor(ptr);
        if(!actor)
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

    bool World::isSubmerged(const MWWorld::ConstPtr &object) const
    {
        return isUnderwater(object, 1.0f/mSwimHeightScale);
    }

    bool World::isSwimming(const MWWorld::ConstPtr &object) const
    {
        return isUnderwater(object, mSwimHeightScale);
    }

    bool World::isWading(const MWWorld::ConstPtr &object) const
    {
        const float kneeDeep = 0.25f;
        return isUnderwater(object, kneeDeep);
    }

    bool World::isUnderwater(const MWWorld::ConstPtr &object, const float heightRatio) const
    {
        osg::Vec3f pos (object.getRefData().getPosition().asVec3());

        pos.z() += heightRatio*2*mPhysics->getRenderingHalfExtents(object).z();

        const CellStore *currCell = object.isInCell() ? object.getCell() : nullptr; // currCell == nullptr should only happen for player, during initial startup

        return isUnderwater(currCell, pos);
    }

    bool World::isUnderwater(const MWWorld::CellStore* cell, const osg::Vec3f &pos) const
    {
        if (!cell)
            return false;

        if (!(cell->getCell()->hasWater())) {
            return false;
        }
        return pos.z() < cell->getWaterLevel();
    }

    bool World::isWaterWalkingCastableOnTarget(const MWWorld::ConstPtr &target) const
    {
        const MWWorld::CellStore* cell = target.getCell();
        if (!cell->getCell()->hasWater())
            return true;

        float waterlevel = cell->getWaterLevel();

        // SwimHeightScale affects the upper z position an actor can swim to
        // while in water. Based on observation from the original engine,
        // the upper z position you get with a +1 SwimHeightScale is the depth
        // limit for being able to cast water walking on an underwater target.
        if (isUnderwater(target, mSwimHeightScale + 1) || (isUnderwater(cell, target.getRefData().getPosition().asVec3()) && !mPhysics->canMoveToWaterSurface(target, waterlevel)))
            return false; // not castable if too deep or if not enough room to move actor to surface
        else
            return true;
    }

    bool World::isOnGround(const MWWorld::Ptr &ptr) const
    {
        return mPhysics->isOnGround(ptr);
    }

    void World::togglePOV(bool force)
    {
        mRendering->togglePOV(force);
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
            mPlayer.reset(new MWWorld::Player(player));
        else
        {
            // Remove the old CharacterController
            MWBase::Environment::get().getMechanicsManager()->remove(getPlayerPtr());
            mNavigator->removeAgent(getPathfindingHalfExtents(getPlayerConstPtr()));
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

        MWWorld::Ptr player = getPlayerPtr();

        mRendering->renderPlayer(player);
        MWRender::NpcAnimation* anim = static_cast<MWRender::NpcAnimation*>(mRendering->getAnimation(player));
        player.getClass().getInventoryStore(player).setInvListener(anim, player);
        player.getClass().getInventoryStore(player).setContListener(anim);

        scaleObject(player, player.getCellRef().getScale()); // apply race height
        rotateObject(player, 0.f, 0.f, 0.f, MWBase::RotationFlag_inverseOrder | MWBase::RotationFlag_adjust);

        MWBase::Environment::get().getMechanicsManager()->add(getPlayerPtr());
        MWBase::Environment::get().getMechanicsManager()->watchActor(getPlayerPtr());

        std::string model = getPlayerPtr().getClass().getModel(getPlayerPtr());
        model = Misc::ResourceHelpers::correctActorModelPath(model, mResourceSystem->getVFS());
        mPhysics->remove(getPlayerPtr());
        mPhysics->addActor(getPlayerPtr(), model);

        applyLoopingParticles(player);

        mDefaultHalfExtents = mPhysics->getOriginalHalfExtents(getPlayerPtr());
        mNavigator->addAgent(getPathfindingHalfExtents(getPlayerConstPtr()));
    }

    World::RestPermitted World::canRest () const
    {
        CellStore *currentCell = mWorldScene->getCurrentCell();

        Ptr player = mPlayer->getPlayer();
        RefData &refdata = player.getRefData();
        osg::Vec3f playerPos(refdata.getPosition().asVec3());

        const MWPhysics::Actor* actor = mPhysics->getActor(player);
        if (!actor)
            throw std::runtime_error("can't find player");

        if(mPlayer->enemiesNearby())
            return Rest_EnemiesAreNearby;

        if (isUnderwater(currentCell, playerPos) || isWalkingOnWater(player))
            return Rest_PlayerIsUnderwater;

        float fallHeight = player.getClass().getCreatureStats(player).getFallHeight();
        float epsilon = 1e-4;
        if ((actor->getCollisionMode() && (!mPhysics->isOnSolidGround(player) || fallHeight >= epsilon)) || isFlying(player))
            return Rest_PlayerIsInAir;

        if((currentCell->getCell()->mData.mFlags&ESM::Cell::NoSleep) || player.getClass().getNpcStats(player).isWerewolf())
            return Rest_OnlyWaiting;

        return Rest_Allowed;
    }

    MWRender::Animation* World::getAnimation(const MWWorld::Ptr &ptr)
    {
        return mRendering->getAnimation(ptr);
    }

    const MWRender::Animation* World::getAnimation(const MWWorld::ConstPtr &ptr) const
    {
        return mRendering->getAnimation(ptr);
    }

    void World::screenshot(osg::Image* image, int w, int h)
    {
        mRendering->screenshot(image, w, h);
    }

    bool World::screenshot360(osg::Image* image, std::string settingStr)
    {
        return mRendering->screenshot360(image,settingStr);
    }

    void World::activateDoor(const MWWorld::Ptr& door)
    {
        auto state = door.getClass().getDoorState(door);
        switch (state)
        {
        case MWWorld::DoorState::Idle:
            if (door.getRefData().getPosition().rot[2] == door.getCellRef().getPosition().rot[2])
                state = MWWorld::DoorState::Opening; // if closed, then open
            else
                state = MWWorld::DoorState::Closing; // if open, then close
            break;
        case MWWorld::DoorState::Closing:
            state = MWWorld::DoorState::Opening; // if closing, then open
            break;
        case MWWorld::DoorState::Opening:
        default:
            state = MWWorld::DoorState::Closing; // if opening, then close
            break;
        }
        door.getClass().setDoorState(door, state);
        mDoorStates[door] = state;
    }

    void World::activateDoor(const Ptr &door, MWWorld::DoorState state)
    {
        door.getClass().setDoorState(door, state);
        mDoorStates[door] = state;
        if (state == MWWorld::DoorState::Idle)
        {
            mDoorStates.erase(door);
            rotateDoor(door, state, 1);
        }
    }

    bool World::getPlayerStandingOn (const MWWorld::ConstPtr& object)
    {
        MWWorld::Ptr player = getPlayerPtr();
        return mPhysics->isActorStandingOn(player, object);
    }

    bool World::getActorStandingOn (const MWWorld::ConstPtr& object)
    {
        std::vector<MWWorld::Ptr> actors;
        mPhysics->getActorsStandingOn(object, actors);
        return !actors.empty();
    }

    void World::getActorsStandingOn (const MWWorld::ConstPtr& object, std::vector<MWWorld::Ptr> &actors)
    {
        mPhysics->getActorsStandingOn(object, actors);
    }

    bool World::getPlayerCollidingWith (const MWWorld::ConstPtr& object)
    {
        MWWorld::Ptr player = getPlayerPtr();
        return mPhysics->isActorCollidingWith(player, object);
    }

    bool World::getActorCollidingWith (const MWWorld::ConstPtr& object)
    {
        std::vector<MWWorld::Ptr> actors;
        mPhysics->getActorsCollidingWith(object, actors);
        return !actors.empty();
    }

    void World::hurtStandingActors(const ConstPtr &object, float healthPerSecond)
    {
        if (MWBase::Environment::get().getWindowManager()->isGuiMode())
            return;

        std::vector<MWWorld::Ptr> actors;
        mPhysics->getActorsStandingOn(object, actors);
        for (const Ptr &actor : actors)
        {
            MWMechanics::CreatureStats& stats = actor.getClass().getCreatureStats(actor);
            if (stats.isDead())
                continue;

            mPhysics->markAsNonSolid (object);

            if (actor == getPlayerPtr() && mGodMode)
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

    void World::hurtCollidingActors(const ConstPtr &object, float healthPerSecond)
    {
        if (MWBase::Environment::get().getWindowManager()->isGuiMode())
            return;

        std::vector<Ptr> actors;
        mPhysics->getActorsCollidingWith(object, actors);
        for (const Ptr &actor : actors)
        {
            MWMechanics::CreatureStats& stats = actor.getClass().getCreatureStats(actor);
            if (stats.isDead())
                continue;

            mPhysics->markAsNonSolid (object);

            if (actor == getPlayerPtr() && mGodMode)
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

    struct GetContainersOwnedByVisitor
    {
        GetContainersOwnedByVisitor(const MWWorld::ConstPtr& owner, std::vector<MWWorld::Ptr>& out)
            : mOwner(owner)
            , mOut(out)
        {
        }

        MWWorld::ConstPtr mOwner;
        std::vector<MWWorld::Ptr>& mOut;

        bool operator()(const MWWorld::Ptr& ptr)
        {
            if (ptr.getRefData().isDeleted())
                return true;

            // vanilla Morrowind does not allow to sell items from containers with zero capacity
            if (ptr.getClass().getCapacity(ptr) <= 0.f)
                return true;

            if (Misc::StringUtils::ciEqual(ptr.getCellRef().getOwner(), mOwner.getCellRef().getRefId()))
                mOut.push_back(ptr);

            return true;
        }
    };

    void World::getContainersOwnedBy (const MWWorld::ConstPtr& owner, std::vector<MWWorld::Ptr>& out)
    {
        for (CellStore* cellstore : mWorldScene->getActiveCells())
        {
            GetContainersOwnedByVisitor visitor (owner, out);
            cellstore->forEachType<ESM::Container>(visitor);
        }
    }

    void World::getItemsOwnedBy (const MWWorld::ConstPtr& npc, std::vector<MWWorld::Ptr>& out)
    {
        for (CellStore* cellstore : mWorldScene->getActiveCells())
        {
            cellstore->forEach([&] (const auto& ptr) {
                if (ptr.getRefData().getBaseNode() && Misc::StringUtils::ciEqual(ptr.getCellRef().getOwner(), npc.getCellRef().getRefId()))
                    out.push_back(ptr);
                return true;
            });
        }
    }

    bool World::getLOS(const MWWorld::ConstPtr& actor, const MWWorld::ConstPtr& targetActor)
    {
        if (!targetActor.getRefData().isEnabled() || !actor.getRefData().isEnabled())
            return false; // cannot get LOS unless both NPC's are enabled
        if (!targetActor.getRefData().getBaseNode() || !actor.getRefData().getBaseNode())
            return false; // not in active cell

        return mPhysics->getLineOfSight(actor, targetActor);
    }

    float World::getDistToNearestRayHit(const osg::Vec3f& from, const osg::Vec3f& dir, float maxDist, bool includeWater)
    {
        osg::Vec3f to (dir);
        to.normalize();
        to = from + (to * maxDist);

        int collisionTypes = MWPhysics::CollisionType_World | MWPhysics::CollisionType_HeightMap | MWPhysics::CollisionType_Door;
        if (includeWater) {
            collisionTypes |= MWPhysics::CollisionType_Water;
        }
        MWPhysics::PhysicsSystem::RayResult result = mPhysics->castRay(from, to, MWWorld::Ptr(), std::vector<MWWorld::Ptr>(), collisionTypes);

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
        pos.rot[0] = pos.rot[1] = pos.rot[2] = 0;
        pos.pos[0] = pos.pos[1] = pos.pos[2] = 0;

        MWWorld::CellStore *cellStore = getInterior(name);

        if (!cellStore)
            return false;

        std::vector<const MWWorld::CellRef *> sortedDoors;
        for (const MWWorld::LiveCellRef<ESM::Door>& door : cellStore->getReadOnlyDoors().mList)
        {
            if (!door.mRef.getTeleport())
                continue;

            sortedDoors.push_back(&door.mRef);
        }

        // Sort teleporting doors alphabetically, first by ID, then by destination cell to make search consistent
        std::sort(sortedDoors.begin(), sortedDoors.end(), [] (const MWWorld::CellRef *lhs, const MWWorld::CellRef *rhs)
        {
            if (lhs->getRefId() != rhs->getRefId())
                return lhs->getRefId() < rhs->getRefId();

            return lhs->getDestCell() < rhs->getDestCell();
        });

        for (const MWWorld::CellRef* door : sortedDoors)
        {
            MWWorld::CellStore *source = nullptr;

            // door to exterior
            if (door->getDestCell().empty())
            {
                int x, y;
                ESM::Position doorDest = door->getDoorDest();
                positionToIndex(doorDest.pos[0], doorDest.pos[1], x, y);
                source = getExterior(x, y);
            }
            // door to interior
            else
            {
                source = getInterior(door->getDestCell());
            }
            if (source)
            {
                // Find door leading to our current teleport door
                // and use its destination to position inside cell.
                for (const MWWorld::LiveCellRef<ESM::Door>& destDoor : source->getReadOnlyDoors().mList)
                {
                    if (Misc::StringUtils::ciEqual(name, destDoor.mRef.getDestCell()))
                    {
                        /// \note Using _any_ door pointed to the interior,
                        /// not the one pointed to current door.
                        pos = destDoor.mRef.getDoorDest();
                        pos.rot[0] = pos.rot[1] = pos.rot[2] = 0;
                        return true;
                    }
                }
            }
        }
        // Fall back to the first static location.
        const MWWorld::CellRefList<ESM::Static>::List &statics = cellStore->getReadOnlyStatics().mList;
        if (!statics.empty())
        {
            pos = statics.begin()->mRef.getPosition();
            pos.rot[0] = pos.rot[1] = pos.rot[2] = 0;
            return true;
        }

        return false;
    }

    bool World::findExteriorPosition(const std::string &name, ESM::Position &pos)
    {
        pos.rot[0] = pos.rot[1] = pos.rot[2] = 0;

        const ESM::Cell *ext = getExterior(name);

        if (!ext && name.find(',') != std::string::npos) {
            try {
                int x = std::stoi(name.substr(0, name.find(',')));
                int y = std::stoi(name.substr(name.find(',')+1));
                ext = getExterior(x, y)->getCell();
            }
            catch (const std::invalid_argument&)
            {
                // This exception can be ignored, as this means that name probably refers to a interior cell instead of comma separated coordinates
            }
            catch (const std::out_of_range&)
            {
                throw std::runtime_error("Cell coordinates out of range.");
            }
        }

        if (ext) {
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
        int idx = 0;
        for (const std::string &file : content)
        {
            boost::filesystem::path filename(file);
            const Files::MultiDirCollection& col = fileCollections.getCollection(filename.extension().string());
            if (col.doesExist(file))
            {
                contentLoader.load(col.getPath(file), idx);
            }
            else
            {
                std::string message = "Failed loading " + file + ": the content file does not exist";
                throw std::runtime_error(message);
            }
            idx++;
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
            const ESM::Spell* spell = mStore.get<ESM::Spell>().find(selectedSpell);

            // Check mana
            bool godmode = (isPlayer && mGodMode);
            MWMechanics::DynamicStat<float> magicka = stats.getMagicka();
            if (magicka.getCurrent() < spell->mData.mCost && !godmode)
            {
                message = "#{sMagicInsufficientSP}";
                fail = true;
            }

            // If this is a power, check if it was already used in the last 24h
            if (!fail && spell->mData.mType == ESM::Spell::ST_Power && !stats.getSpells().canUsePower(spell))
            {
                message = "#{sPowerAlreadyUsed}";
                fail = true;
            }

            // Reduce mana
            if (!fail && !godmode)
            {
                magicka.setCurrent(magicka.getCurrent() - spell->mData.mCost);
                stats.setMagicka(magicka);
            }
        }

        if (isPlayer && fail)
            MWBase::Environment::get().getWindowManager()->messageBox(message);

        return !fail;
    }

    void World::castSpell(const Ptr &actor, bool manualSpell)
    {
        MWMechanics::CreatureStats& stats = actor.getClass().getCreatureStats(actor);

        // For AI actors, get combat targets to use in the ray cast. Only those targets will return a positive hit result.
        std::vector<MWWorld::Ptr> targetActors;
        if (!actor.isEmpty() && actor != MWMechanics::getPlayer() && !manualSpell)
            stats.getAiSequence().getCombatTargets(targetActors);

        const float fCombatDistance = mStore.get<ESM::GameSetting>().find("fCombatDistance")->mValue.getFloat();

        osg::Vec3f hitPosition = actor.getRefData().getPosition().asVec3();

        // for player we can take faced object first
        MWWorld::Ptr target;
        if (actor == MWMechanics::getPlayer())
            target = getFacedObject();

        // if the faced object can not be activated, do not use it
        if (!target.isEmpty() && !target.getClass().hasToolTip(target))
            target = nullptr;

        if (target.isEmpty())
        {
            // For scripted spells we should not use hit contact
            if (manualSpell)
            {
                if (actor != MWMechanics::getPlayer())
                {
                    for (const MWMechanics::AiPackage* package : stats.getAiSequence())
                    {
                        if (package->getTypeId() == MWMechanics::AiPackage::TypeIdCast)
                        {
                            target = package->getTarget();
                            break;
                        }
                    }
                }
            }
            else
            {
                // For actor targets, we want to use hit contact with bounding boxes.
                // This is to give a slight tolerance for errors, especially with creatures like the Skeleton that would be very hard to aim at otherwise.
                // For object targets, we want the detailed shapes (rendering raycast).
                // If we used the bounding boxes for static objects, then we would not be able to target e.g. objects lying on a shelf.
                std::pair<MWWorld::Ptr,osg::Vec3f> result1 = getHitContact(actor, fCombatDistance, targetActors);

                // Get the target to use for "on touch" effects, using the facing direction from Head node
                osg::Vec3f origin = getActorHeadTransform(actor).getTrans();

                osg::Quat orient = osg::Quat(actor.getRefData().getPosition().rot[0], osg::Vec3f(-1,0,0))
                        * osg::Quat(actor.getRefData().getPosition().rot[2], osg::Vec3f(0,0,-1));

                osg::Vec3f direction = orient * osg::Vec3f(0,1,0);
                float distance = getMaxActivationDistance();
                osg::Vec3f dest = origin + direction * distance;

                MWRender::RenderingManager::RayResult result2 = mRendering->castRay(origin, dest, true, true);

                float dist1 = std::numeric_limits<float>::max();
                float dist2 = std::numeric_limits<float>::max();

                if (!result1.first.isEmpty() && result1.first.getClass().isActor())
                    dist1 = (origin - result1.second).length();
                if (result2.mHit)
                    dist2 = (origin - result2.mHitPointWorld).length();

                if (!result1.first.isEmpty() && result1.first.getClass().isActor())
                {
                    target = result1.first;
                    hitPosition = result1.second;
                    if (dist1 > getMaxActivationDistance())
                        target = nullptr;
                }
                else if (result2.mHit)
                {
                    target = result2.mHitObject;
                    hitPosition = result2.mHitPointWorld;
                    if (dist2 > getMaxActivationDistance() && !target.isEmpty() && !target.getClass().hasToolTip(target))
                        target = nullptr;
                }
            }
        }

        std::string selectedSpell = stats.getSpells().getSelectedSpell();

        MWMechanics::CastSpell cast(actor, target, false, manualSpell);
        cast.mHitPosition = hitPosition;

        if (!selectedSpell.empty())
        {
            const ESM::Spell* spell = mStore.get<ESM::Spell>().find(selectedSpell);
            cast.cast(spell);
        }
        else if (actor.getClass().hasInventoryStore(actor))
        {
            MWWorld::InventoryStore& inv = actor.getClass().getInventoryStore(actor);
            if (inv.getSelectedEnchantItem() != inv.end())
                cast.cast(*inv.getSelectedEnchantItem());
        }
    }

    void World::launchProjectile (MWWorld::Ptr& actor, MWWorld::Ptr& projectile,
                                   const osg::Vec3f& worldPos, const osg::Quat& orient, MWWorld::Ptr& bow, float speed, float attackStrength)
    {
        // An initial position of projectile can be outside shooter's collision box, so any object between shooter and launch position will be ignored.
        // To avoid this issue, we should check for impact immediately before launch the projectile.
        // So we cast a 1-yard-length ray from shooter to launch position and check if there are collisions in this area.
        // TODO: as a better solutuon we should handle projectiles during physics update, not during world update.
        const osg::Vec3f sourcePos = worldPos + orient * osg::Vec3f(0,-1,0) * 64.f;

        // Early out if the launch position is underwater
        bool underwater = MWBase::Environment::get().getWorld()->isUnderwater(MWMechanics::getPlayer().getCell(), worldPos);
        if (underwater)
        {
            mRendering->emitWaterRipple(worldPos);
            return;
        }

        // For AI actors, get combat targets to use in the ray cast. Only those targets will return a positive hit result.
        std::vector<MWWorld::Ptr> targetActors;
        if (!actor.isEmpty() && actor.getClass().isActor() && actor != MWMechanics::getPlayer())
            actor.getClass().getCreatureStats(actor).getAiSequence().getCombatTargets(targetActors);

        // Check for impact, if yes, handle hit, if not, launch projectile
        MWPhysics::PhysicsSystem::RayResult result = mPhysics->castRay(sourcePos, worldPos, actor, targetActors, 0xff, MWPhysics::CollisionType_Projectile);
        if (result.mHit)
            MWMechanics::projectileHit(actor, result.mHitObject, bow, projectile, result.mHitPos, attackStrength);
        else
            mProjectileManager->launchProjectile(actor, projectile, worldPos, orient, bow, speed, attackStrength);
    }

    void World::launchMagicBolt (const std::string &spellId, const MWWorld::Ptr& caster, const osg::Vec3f& fallbackDirection)
    {
        mProjectileManager->launchMagicBolt(spellId, caster, fallbackDirection);
    }

    void World::applyLoopingParticles(const MWWorld::Ptr& ptr)
    {
        const MWWorld::Class &cls = ptr.getClass();
        if (cls.isActor())
        {
            MWMechanics::ApplyLoopingParticlesVisitor visitor(ptr);
            cls.getCreatureStats(ptr).getActiveSpells().visitEffectSources(visitor);
            cls.getCreatureStats(ptr).getSpells().visitEffectSources(visitor);
            if (cls.hasInventoryStore(ptr))
                cls.getInventoryStore(ptr).visitEffectSources(visitor);
        }
    }

    const std::vector<std::string>& World::getContentFiles() const
    {
        return mContentFiles;
    }

    void World::breakInvisibility(const Ptr &actor)
    {
        actor.getClass().getCreatureStats(actor).getSpells().purgeEffect(ESM::MagicEffect::Invisibility);
        actor.getClass().getCreatureStats(actor).getActiveSpells().purgeEffect(ESM::MagicEffect::Invisibility);
        if (actor.getClass().hasInventoryStore(actor))
            actor.getClass().getInventoryStore(actor).purgeEffect(ESM::MagicEffect::Invisibility);

        // Normally updated once per frame, but here it is kinda important to do it right away.
        MWBase::Environment::get().getMechanicsManager()->updateMagicEffects(actor);
    }

    bool World::useTorches() const
    {
        // If we are in exterior, check the weather manager.
        // In interiors there are no precipitations and sun, so check the ambient
        // Looks like pseudo-exteriors considered as interiors in this case
        MWWorld::CellStore* cell = mPlayer->getPlayer().getCell();
        if (cell->isExterior())
        {
            float hour = getTimeStamp().getHour();
            return mWeatherManager->useTorches(hour);
        }
        else
        {
            uint32_t ambient = cell->getCell()->mAmbi.mAmbient;
            int ambientTotal = (ambient & 0xff)
                    + ((ambient>>8) & 0xff)
                    + ((ambient>>16) & 0xff);
            return !(cell->getCell()->mData.mFlags & ESM::Cell::NoSleep) && ambientTotal <= 201;
        }
    }

    bool World::findInteriorPositionInWorldSpace(const MWWorld::CellStore* cell, osg::Vec3f& result)
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
            for (const std::string &currentCell : currentCells)
            {
                MWWorld::CellStore *next = getInterior(currentCell);
                if ( !next ) continue;

                // Check if any door in the cell leads to an exterior directly
                for (const MWWorld::LiveCellRef<ESM::Door>& ref : next->getReadOnlyDoors().mList)
                {
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

                checkedCells.insert(currentCell);
            }
        }

        // No luck :(
        return false;
    }

    MWWorld::ConstPtr World::getClosestMarker( const MWWorld::Ptr &ptr, const std::string &id )
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
        MWWorld::ConstPtr closestMarker;

        nextCells.insert( ptr.getCell()->getCell()->mName );
        while ( !nextCells.empty() ) {
            currentCells = nextCells;
            nextCells.clear();
            for (const std::string &cell : currentCells) {
                MWWorld::CellStore *next = getInterior(cell);
                checkedCells.insert(cell);
                if ( !next ) continue;

                closestMarker = next->searchConst( id );
                if ( !closestMarker.isEmpty() )
                {
                    return closestMarker;
                }

                // Check if any door in the cell leads to an exterior directly
                for (const MWWorld::LiveCellRef<ESM::Door>& ref : next->getReadOnlyDoors().mList)
                {
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

    MWWorld::ConstPtr World::getClosestMarkerFromExteriorPosition( const osg::Vec3f& worldPos, const std::string &id ) {
        MWWorld::ConstPtr closestMarker;
        float closestDistance = std::numeric_limits<float>::max();

        std::vector<MWWorld::Ptr> markers;
        mCells.getExteriorPtrs(id, markers);
        for (const Ptr& marker : markers)
        {
            osg::Vec3f markerPos = marker.getRefData().getPosition().asVec3();
            float distance = (worldPos - markerPos).length2();
            if (distance < closestDistance)
            {
                closestDistance = distance;
                closestMarker = marker;
            }
        }

        return closestMarker;
    }

    void World::rest(double hours)
    {
        mCells.rest(hours);
    }

    void World::rechargeItems(double duration, bool activeOnly)
    {
        MWWorld::Ptr player = getPlayerPtr();
        player.getClass().getInventoryStore(player).rechargeItems(duration);

        if (activeOnly)
        {
            for (auto &cell : mWorldScene->getActiveCells())
            {
                cell->recharge(duration);
            }
        }
        else
            mCells.recharge(duration);
    }

    void World::teleportToClosestMarker (const MWWorld::Ptr& ptr,
                                          const std::string& id)
    {
        MWWorld::ConstPtr closestMarker = getClosestMarker( ptr, id );

        if ( closestMarker.isEmpty() )
        {
            Log(Debug::Warning) << "Failed to teleport: no closest marker found";
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
        bool isExterior = isCellExterior() || isCellQuasiExterior();
        if (mPlayer->wasTeleported())
        {
            mPlayer->setTeleported(false);

            const std::string playerRegion = Misc::StringUtils::lowerCase(getPlayerPtr().getCell()->getCell()->mRegion);
            mWeatherManager->playerTeleported(playerRegion, isExterior);
        }

        const TimeStamp time = getTimeStamp();
        mWeatherManager->update(duration, paused, time, isExterior);
    }

    struct AddDetectedReferenceVisitor
    {
        AddDetectedReferenceVisitor(std::vector<Ptr>& out, const Ptr& detector, World::DetectionType type, float squaredDist)
            : mOut(out), mDetector(detector), mSquaredDist(squaredDist), mType(type)
        {
        }

        std::vector<Ptr>& mOut;
        Ptr mDetector;
        float mSquaredDist;
        World::DetectionType mType;
        bool operator() (const MWWorld::Ptr& ptr)
        {
            if ((ptr.getRefData().getPosition().asVec3() - mDetector.getRefData().getPosition().asVec3()).length2() >= mSquaredDist)
                return true;

            if (!ptr.getRefData().isEnabled() || ptr.getRefData().isDeleted())
                return true;

            // Consider references inside containers as well (except if we are looking for a Creature, they cannot be in containers)
            bool isContainer = ptr.getClass().getTypeName() == typeid(ESM::Container).name();
            if (mType != World::Detect_Creature && (ptr.getClass().isActor() || isContainer))
            {
                // but ignore containers without resolved content
                if (isContainer && ptr.getRefData().getCustomData() == nullptr)
                    return true;

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

        bool needToAdd (const MWWorld::Ptr& ptr, const MWWorld::Ptr& detector)
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

        AddDetectedReferenceVisitor visitor (out, ptr, type, dist*dist);

        for (CellStore* cellStore : mWorldScene->getActiveCells())
        {
            cellStore->forEach(visitor);
        }
    }

    float World::feetToGameUnits(float feet)
    {
        // Original engine rounds size upward
        static const int unitsPerFoot = ceil(Constants::UnitsPerFoot);
        return feet * unitsPerFoot;
    }

    float World::getActivationDistancePlusTelekinesis()
    {
        float telekinesisRangeBonus =
                    mPlayer->getPlayer().getClass().getCreatureStats(mPlayer->getPlayer()).getMagicEffects()
                    .get(ESM::MagicEffect::Telekinesis).getMagnitude();
        telekinesisRangeBonus = feetToGameUnits(telekinesisRangeBonus);

        float activationDistance = getMaxActivationDistance() + telekinesisRangeBonus;

        return activationDistance;
    }

    MWWorld::Ptr World::getPlayerPtr()
    {
        return mPlayer->getPlayer();
    }

    MWWorld::ConstPtr World::getPlayerConstPtr() const
    {
        return mPlayer->getConstPlayer();
    }

    void World::updateDialogueGlobals()
    {
        MWWorld::Ptr player = getPlayerPtr();
        int bounty = player.getClass().getNpcStats(player).getBounty();
        int playerGold = player.getClass().getContainerStore(player).count(ContainerStore::sGoldId);

        static float fCrimeGoldDiscountMult = mStore.get<ESM::GameSetting>().find("fCrimeGoldDiscountMult")->mValue.getFloat();
        static float fCrimeGoldTurnInMult = mStore.get<ESM::GameSetting>().find("fCrimeGoldTurnInMult")->mValue.getFloat();

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
        MWWorld::ConstPtr prisonMarker = getClosestMarker( ptr, "prisonmarker" );
        if ( prisonMarker.isEmpty() )
        {
            Log(Debug::Warning) << "Failed to confiscate items: no closest prison marker found.";
            return;
        }
        std::string prisonName = prisonMarker.getCellRef().getDestCell();
        if ( prisonName.empty() )
        {
            Log(Debug::Warning) << "Failed to confiscate items: prison marker not linked to prison interior";
            return;
        }
        MWWorld::CellStore *prison = getInterior( prisonName );
        if ( !prison )
        {
            Log(Debug::Warning) << "Failed to confiscate items: failed to load cell " << prisonName;
            return;
        }

        MWWorld::Ptr closestChest = prison->search( "stolen_goods" );
        if (!closestChest.isEmpty()) //Found a close chest
        {
            MWBase::Environment::get().getMechanicsManager()->confiscateStolenItems(ptr, closestChest);
        }
        else
           Log(Debug::Warning) << "Failed to confiscate items: no stolen_goods container found";
    }

    void World::goToJail()
    {
        if (!mGoToJail)
        {
            // Reset bounty and forget the crime now, but don't change cell yet (the player should be able to read the dialog text first)
            mGoToJail = true;
            mPlayerInJail = true;

            MWWorld::Ptr player = getPlayerPtr();

            int bounty = player.getClass().getNpcStats(player).getBounty();
            player.getClass().getNpcStats(player).setBounty(0);
            mPlayer->recordCrimeId();
            confiscateStolenItems(player);

            static int iDaysinPrisonMod = mStore.get<ESM::GameSetting>().find("iDaysinPrisonMod")->mValue.getInteger();
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

    bool World::isPlayerInJail() const
    {
        return mPlayerInJail;
    }

    void World::setPlayerTraveling(bool traveling)
    {
        mPlayerTraveling = traveling;
    }

    bool World::isPlayerTraveling() const
    {
        return mPlayerTraveling;
    }

    float World::getTerrainHeightAt(const osg::Vec3f& worldPos) const
    {
        return mRendering->getTerrainHeightAt(worldPos);
    }

    osg::Vec3f World::getHalfExtents(const ConstPtr& object, bool rendering) const
    {
        if (!object.getClass().isActor())
            return mRendering->getHalfExtents(object);

        // Handle actors separately because of bodyparts
        if (rendering)
            return mPhysics->getRenderingHalfExtents(object);
        else
            return mPhysics->getHalfExtents(object);
    }

    std::string World::exportSceneGraph(const Ptr &ptr)
    {
        std::string file = mUserDataPath + "/openmw.osgt";
        mRendering->exportSceneGraph(ptr, file, "Ascii");
        return file;
    }

    void World::spawnRandomCreature(const std::string &creatureList)
    {
        const ESM::CreatureLevList* list = mStore.get<ESM::CreatureLevList>().find(creatureList);

        static int iNumberCreatures = mStore.get<ESM::GameSetting>().find("iNumberCreatures")->mValue.getInteger();
        int numCreatures = 1 + Misc::Rng::rollDice(iNumberCreatures); // [1, iNumberCreatures]

        for (int i=0; i<numCreatures; ++i)
        {
            std::string selectedCreature = MWMechanics::getLevelledItem(list, true);
            if (selectedCreature.empty())
                continue;

            MWWorld::ManualRef ref(mStore, selectedCreature, 1);

            safePlaceObject(ref.getPtr(), getPlayerPtr(), getPlayerPtr().getCell(), 0, 220.f);
        }
    }

    void World::spawnBloodEffect(const Ptr &ptr, const osg::Vec3f &worldPosition)
    {
        if (ptr == getPlayerPtr() && Settings::Manager::getBool("hit fader", "GUI"))
            return;

        std::string texture = Fallback::Map::getString("Blood_Texture_" + std::to_string(ptr.getClass().getBloodTexture(ptr)));
        if (texture.empty())
            texture = Fallback::Map::getString("Blood_Texture_0");

        std::string model = "meshes\\" + Fallback::Map::getString("Blood_Model_" + std::to_string(Misc::Rng::rollDice(3))); // [0, 2]

        mRendering->spawnEffect(model, texture, worldPosition, 1.0f, false);
    }

    void World::spawnEffect(const std::string &model, const std::string &textureOverride, const osg::Vec3f &worldPos, float scale, bool isMagicVFX)
    {
        mRendering->spawnEffect(model, textureOverride, worldPos, scale, isMagicVFX);
    }

    void World::explodeSpell(const osg::Vec3f& origin, const ESM::EffectList& effects, const Ptr& caster, const Ptr& ignore, ESM::RangeType rangeType,
                             const std::string& id, const std::string& sourceName, const bool fromProjectile)
    {
        std::map<MWWorld::Ptr, std::vector<ESM::ENAMstruct> > toApply;
        for (const ESM::ENAMstruct& effectInfo : effects.mList)
        {
            const ESM::MagicEffect* effect = mStore.get<ESM::MagicEffect>().find(effectInfo.mEffectID);

            if (effectInfo.mRange != rangeType || (effectInfo.mArea <= 0 && !ignore.isEmpty() && ignore.getClass().isActor()))
                continue; // Not right range type, or not area effect and hit an actor

            if (fromProjectile && effectInfo.mArea <= 0)
                continue; // Don't play explosion for projectiles with 0-area effects

            if (!fromProjectile && effectInfo.mRange == ESM::RT_Touch && !ignore.isEmpty() && !ignore.getClass().isActor() && !ignore.getClass().hasToolTip(ignore))
                continue; // Don't play explosion for touch spells on non-activatable objects except when spell is from the projectile enchantment

            // Spawn the explosion orb effect
            const ESM::Static* areaStatic;
            if (!effect->mArea.empty())
                areaStatic = mStore.get<ESM::Static>().find (effect->mArea);
            else
                areaStatic = mStore.get<ESM::Static>().find ("VFX_DefaultArea");

            std::string texture = effect->mParticle;

            if (effectInfo.mArea <= 0)
            {
                if (effectInfo.mRange == ESM::RT_Target)
                    mRendering->spawnEffect("meshes\\" + areaStatic->mModel, texture, origin, 1.0f);
                continue;
            }
            else
                mRendering->spawnEffect("meshes\\" + areaStatic->mModel, texture, origin, static_cast<float>(effectInfo.mArea * 2));

            // Play explosion sound (make sure to use NoTrack, since we will delete the projectile now)
            static const std::string schools[] = {
                "alteration", "conjuration", "destruction", "illusion", "mysticism", "restoration"
            };
            {
                MWBase::SoundManager *sndMgr = MWBase::Environment::get().getSoundManager();
                if(!effect->mAreaSound.empty())
                    sndMgr->playSound3D(origin, effect->mAreaSound, 1.0f, 1.0f);
                else
                    sndMgr->playSound3D(origin, schools[effect->mData.mSchool]+" area", 1.0f, 1.0f);
            }
            // Get the actors in range of the effect
            std::vector<MWWorld::Ptr> objects;
            MWBase::Environment::get().getMechanicsManager()->getObjectsInRange(
                        origin, feetToGameUnits(static_cast<float>(effectInfo.mArea)), objects);
            for (const Ptr& affected : objects)
            {
                // Ignore actors without collisions here, otherwise it will be possible to hit actors outside processing range.
                if (affected.getClass().isActor() && !isActorCollisionEnabled(affected))
                    continue;

                toApply[affected].push_back(effectInfo);
            }
        }

        // Now apply the appropriate effects to each actor in range
        for (auto& applyPair : toApply)
        {
            MWWorld::Ptr source = caster;
            // Vanilla-compatible behaviour of never applying the spell to the caster
            // (could be changed by mods later)
            if (applyPair.first == caster)
                continue;

            if (applyPair.first == ignore)
                continue;

            if (source.isEmpty())
                source = applyPair.first;

            MWMechanics::CastSpell cast(source, applyPair.first);
            cast.mHitPosition = origin;
            cast.mId = id;
            cast.mSourceName = sourceName;
            cast.mStack = false;
            ESM::EffectList effectsToApply;
            effectsToApply.mList = applyPair.second;
            cast.inflict(applyPair.first, caster, effectsToApply, rangeType, false, true);
        }
    }

    void World::activate(const Ptr &object, const Ptr &actor)
    {
        breakInvisibility(actor);

        if (object.getRefData().activate())
        {
            std::shared_ptr<MWWorld::Action> action = (object.getClass().activate(object, actor));
            action->execute (actor);
        }
    }

    struct ResetActorsVisitor
    {
        bool operator() (Ptr ptr)
        {
            if (ptr.getClass().isActor() && ptr.getCellRef().hasContentFile())
            {
                if (ptr.getCell()->movedHere(ptr))
                    return true;

                const ESM::Position& origPos = ptr.getCellRef().getPosition();
                MWBase::Environment::get().getWorld()->moveObject(ptr, origPos.pos[0], origPos.pos[1], origPos.pos[2]);
                MWBase::Environment::get().getWorld()->rotateObject(ptr, origPos.rot[0], origPos.rot[1], origPos.rot[2]);
                ptr.getClass().adjustPosition(ptr, true);
            }
            return true;
        }
    };
    void World::resetActors()
    {
        for (CellStore* cellstore : mWorldScene->getActiveCells())
        {
            ResetActorsVisitor visitor;
            cellstore->forEach(visitor);
        }
    }

    bool World::isWalkingOnWater(const ConstPtr &actor) const
    {
        const MWPhysics::Actor* physicActor = mPhysics->getActor(actor);
        if (physicActor && physicActor->isWalkingOnWater())
            return true;
        return false;
    }

    osg::Vec3f World::aimToTarget(const ConstPtr &actor, const MWWorld::ConstPtr& target)
    {
        osg::Vec3f weaponPos = actor.getRefData().getPosition().asVec3();
        weaponPos.z() += mPhysics->getHalfExtents(actor).z();
        osg::Vec3f targetPos = mPhysics->getCollisionObjectPosition(target);
        return (targetPos - weaponPos);
    }

    float World::getHitDistance(const ConstPtr &actor, const ConstPtr &target)
    {
        osg::Vec3f weaponPos = actor.getRefData().getPosition().asVec3();
        osg::Vec3f halfExtents = mPhysics->getHalfExtents(actor);
        weaponPos.z() += halfExtents.z();

        return mPhysics->getHitDistance(weaponPos, target) - halfExtents.y();
    }

    void preload(MWWorld::Scene* scene, const ESMStore& store, const std::string& obj)
    {
        if (obj.empty())
            return;
        try
        {
            MWWorld::ManualRef ref(store, obj);
            std::string model = ref.getPtr().getClass().getModel(ref.getPtr());
            if (!model.empty())
                scene->preload(model, ref.getPtr().getClass().useAnim());
        }
        catch(std::exception& e)
        {
        }
    }

    void World::preloadEffects(const ESM::EffectList *effectList)
    {
        for (const ESM::ENAMstruct& effectInfo : effectList->mList)
        {
            const ESM::MagicEffect *effect = mStore.get<ESM::MagicEffect>().find(effectInfo.mEffectID);

            if (MWMechanics::isSummoningEffect(effectInfo.mEffectID))
            {
                preload(mWorldScene.get(), mStore, "VFX_Summon_Start");
                preload(mWorldScene.get(), mStore, MWMechanics::getSummonedCreature(effectInfo.mEffectID));
            }

            preload(mWorldScene.get(), mStore, effect->mCasting);
            preload(mWorldScene.get(), mStore, effect->mHit);

            if (effectInfo.mArea > 0)
                preload(mWorldScene.get(), mStore, effect->mArea);
            if (effectInfo.mRange == ESM::RT_Target)
                preload(mWorldScene.get(), mStore, effect->mBolt);
        }
    }

    DetourNavigator::Navigator* World::getNavigator() const
    {
        return mNavigator.get();
    }

    void World::updateActorPath(const MWWorld::ConstPtr& actor, const std::deque<osg::Vec3f>& path,
            const osg::Vec3f& halfExtents, const osg::Vec3f& start, const osg::Vec3f& end) const
    {
        mRendering->updateActorPath(actor, path, halfExtents, start, end);
    }

    void World::removeActorPath(const MWWorld::ConstPtr& actor) const
    {
        mRendering->removeActorPath(actor);
    }

    void World::setNavMeshNumberToRender(const std::size_t value)
    {
        mRendering->setNavMeshNumber(value);
    }

    osg::Vec3f World::getPathfindingHalfExtents(const MWWorld::ConstPtr& actor) const
    {
        if (actor.isInCell() && actor.getCell()->isExterior())
            return mDefaultHalfExtents; // Using default half extents for better performance
        else
            return getHalfExtents(actor);
    }

    bool World::hasCollisionWithDoor(const MWWorld::ConstPtr& door, const osg::Vec3f& position, const osg::Vec3f& destination) const
    {
        const auto object = mPhysics->getObject(door);

        if (!object)
            return false;

        btVector3 aabbMin;
        btVector3 aabbMax;
        object->getShapeInstance()->getCollisionShape()->getAabb(btTransform::getIdentity(), aabbMin, aabbMax);

        const auto toLocal = object->getCollisionObject()->getWorldTransform().inverse();
        const auto localFrom = toLocal(Misc::Convert::toBullet(position));
        const auto localTo = toLocal(Misc::Convert::toBullet(destination));

        btScalar hitDistance = 1;
        btVector3 hitNormal;
        return btRayAabb(localFrom, localTo, aabbMin, aabbMax, hitDistance, hitNormal);
    }

    bool World::isAreaOccupiedByOtherActor(const osg::Vec3f& position, const float radius, const MWWorld::ConstPtr& ignore) const
    {
        return mPhysics->isAreaOccupiedByOtherActor(position, radius, ignore);
    }
}
