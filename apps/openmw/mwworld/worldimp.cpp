#include "worldimp.hpp"

#include <charconv>
#include <vector>

#include <osg/ComputeBoundsVisitor>
#include <osg/Group>
#include <osg/Timer>

#include <MyGUI_TextIterator.h>

#include <LinearMath/btAabbUtil2.h>

#include <components/debug/debuglog.hpp>

#include <components/esm3/cellref.hpp>
#include <components/esm3/esmreader.hpp>
#include <components/esm3/esmwriter.hpp>
#include <components/esm3/loadclas.hpp>
#include <components/esm3/loadcrea.hpp>
#include <components/esm3/loadench.hpp>
#include <components/esm3/loadgmst.hpp>
#include <components/esm3/loadlevlist.hpp>
#include <components/esm3/loadmgef.hpp>
#include <components/esm3/loadregn.hpp>
#include <components/esm3/loadstat.hpp>
#include <components/esm4/loadcell.hpp>
#include <components/esm4/loaddoor.hpp>
#include <components/esm4/loadstat.hpp>
#include <components/esm4/loadwrld.hpp>

#include <components/misc/constants.hpp>
#include <components/misc/convert.hpp>
#include <components/misc/mathutil.hpp>
#include <components/misc/resourcehelpers.hpp>
#include <components/misc/rng.hpp>

#include <components/files/collections.hpp>

#include <components/resource/bulletshape.hpp>
#include <components/resource/resourcesystem.hpp>

#include <components/sceneutil/lightmanager.hpp>
#include <components/sceneutil/positionattitudetransform.hpp>
#include <components/sceneutil/workqueue.hpp>

#include <components/detournavigator/agentbounds.hpp>
#include <components/detournavigator/debug.hpp>
#include <components/detournavigator/navigator.hpp>
#include <components/detournavigator/settings.hpp>
#include <components/detournavigator/stats.hpp>
#include <components/detournavigator/updateguard.hpp>

#include <components/files/conversion.hpp>
#include <components/loadinglistener/loadinglistener.hpp>

#include <components/settings/values.hpp>

#include "../mwbase/environment.hpp"
#include "../mwbase/luamanager.hpp"
#include "../mwbase/mechanicsmanager.hpp"
#include "../mwbase/scriptmanager.hpp"
#include "../mwbase/soundmanager.hpp"
#include "../mwbase/statemanager.hpp"
#include "../mwbase/windowmanager.hpp"

#include "../mwmechanics/actorutil.hpp"
#include "../mwmechanics/aiavoiddoor.hpp" //Used to tell actors to avoid doors
#include "../mwmechanics/combat.hpp"
#include "../mwmechanics/creaturestats.hpp"
#include "../mwmechanics/levelledlist.hpp"
#include "../mwmechanics/npcstats.hpp"
#include "../mwmechanics/spellcasting.hpp"
#include "../mwmechanics/spellutil.hpp"
#include "../mwmechanics/summoning.hpp"

#include "../mwrender/animation.hpp"
#include "../mwrender/camera.hpp"
#include "../mwrender/npcanimation.hpp"
#include "../mwrender/postprocessor.hpp"
#include "../mwrender/renderingmanager.hpp"
#include "../mwrender/vismask.hpp"

#include "../mwscript/globalscripts.hpp"

#include "../mwclass/door.hpp"

#include "../mwphysics/actor.hpp"
#include "../mwphysics/collisiontype.hpp"
#include "../mwphysics/object.hpp"
#include "../mwphysics/physicssystem.hpp"

#include "../mwsound/constants.hpp"

#include "actionteleport.hpp"
#include "cellstore.hpp"
#include "containerstore.hpp"
#include "datetimemanager.hpp"
#include "inventorystore.hpp"
#include "manualref.hpp"
#include "player.hpp"
#include "projectilemanager.hpp"
#include "weather.hpp"

#include "contentloader.hpp"
#include "esmloader.hpp"

namespace MWWorld
{
    namespace
    {
        std::vector<std::pair<std::string_view, ESM::Variant>> generateDefaultGameSettings()
        {
            return {
                // Companion (tribunal)
                { "sCompanionShare", ESM::Variant("Companion Share") },
                { "sCompanionWarningMessage", ESM::Variant("Warning message") },
                { "sCompanionWarningButtonOne", ESM::Variant("Button 1") },
                { "sCompanionWarningButtonTwo", ESM::Variant("Button 2") },
                { "sProfitValue", ESM::Variant("Profit Value") },
                { "sTeleportDisabled", ESM::Variant("Teleport disabled") },
                { "sLevitateDisabled", ESM::Variant("Levitate disabled") },
                // Missing in unpatched MW 1.0
                { "sDifficulty", ESM::Variant("Difficulty") },
                { "fDifficultyMult", ESM::Variant(5.f) },
                { "sAuto_Run", ESM::Variant("Auto Run") },
                { "sServiceRefusal", ESM::Variant("Service Refusal") },
                { "sNeedOneSkill", ESM::Variant("Need one skill") },
                { "sNeedTwoSkills", ESM::Variant("Need two skills") },
                { "sEasy", ESM::Variant("Easy") },
                { "sHard", ESM::Variant("Hard") },
                { "sDeleteNote", ESM::Variant("Delete Note") },
                { "sEditNote", ESM::Variant("Edit Note") },
                { "sAdmireSuccess", ESM::Variant("Admire Success") },
                { "sAdmireFail", ESM::Variant("Admire Fail") },
                { "sIntimidateSuccess", ESM::Variant("Intimidate Success") },
                { "sIntimidateFail", ESM::Variant("Intimidate Fail") },
                { "sTauntSuccess", ESM::Variant("Taunt Success") },
                { "sTauntFail", ESM::Variant("Taunt Fail") },
                { "sBribeSuccess", ESM::Variant("Bribe Success") },
                { "sBribeFail", ESM::Variant("Bribe Fail") },
                { "fNPCHealthBarTime", ESM::Variant(5.f) },
                { "fNPCHealthBarFade", ESM::Variant(1.f) },
                { "fFleeDistance", ESM::Variant(3000.f) },
                { "sMaxSale", ESM::Variant("Max Sale") },
                { "sAnd", ESM::Variant("and") },
                // Werewolf (BM)
                { "fWereWolfRunMult", ESM::Variant(1.3f) },
                { "fWereWolfSilverWeaponDamageMult", ESM::Variant(2.f) },
                { "iWerewolfFightMod", ESM::Variant(100) },
                { "iWereWolfFleeMod", ESM::Variant(100) },
                { "iWereWolfLevelToAttack", ESM::Variant(20) },
                { "iWereWolfBounty", ESM::Variant(1000) },
                { "fCombatDistanceWerewolfMod", ESM::Variant(0.3f) },
            };
        }

        std::vector<std::pair<GlobalVariableName, ESM::Variant>> generateDefaultGlobals()
        {
            return {
                // vanilla Morrowind does not define dayspassed.
                { Globals::sDaysPassed, ESM::Variant(1) }, // but the addons start counting at 1 :(
                { Globals::sWerewolfClawMult, ESM::Variant(25.f) },
                { Globals::sPCKnownWerewolf, ESM::Variant(0) },
                // following should exist in all versions of MW, but not necessarily in TCs
                { Globals::sGameHour, ESM::Variant(0) },
                { Globals::sTimeScale, ESM::Variant(30.f) },
                { Globals::sDay, ESM::Variant(1) },
                { Globals::sYear, ESM::Variant(1) },
                { Globals::sPCRace, ESM::Variant(0) },
                { Globals::sPCHasCrimeGold, ESM::Variant(0) },
                { Globals::sCrimeGoldDiscount, ESM::Variant(0) },
                { Globals::sCrimeGoldTurnIn, ESM::Variant(0) },
                { Globals::sPCHasTurnIn, ESM::Variant(0) },
            };
        }

        std::vector<std::pair<std::string_view, std::string_view>> generateDefaultStatics()
        {
            return {
                // Total conversions from SureAI lack marker records
                { "divinemarker", "marker_divine.nif" },
                { "doormarker", "marker_arrow.nif" },
                { "northmarker", "marker_north.nif" },
                { "templemarker", "marker_temple.nif" },
                { "travelmarker", "marker_travel.nif" },
            };
        }

        std::vector<std::pair<std::string_view, std::string_view>> generateDefaultDoors()
        {
            return { { "prisonmarker", "marker_prison.nif" } };
        }
    }

    struct GameContentLoader : public ContentLoader
    {
        void addLoader(std::string&& extension, ContentLoader& loader)
        {
            mLoaders.emplace(std::move(extension), &loader);
        }

        void load(const std::filesystem::path& filepath, int& index, Loading::Listener* listener) override
        {
            const auto it
                = mLoaders.find(Misc::StringUtils::lowerCase(Files::pathToUnicodeString(filepath.extension())));
            if (it != mLoaders.end())
            {
                const auto filename = filepath.filename();
                Log(Debug::Info) << "Loading content file " << filename;
                if (listener != nullptr)
                    listener->setLabel(MyGUI::TextIterator::toTagsString(Files::pathToUnicodeString(filename)));
                it->second->load(filepath, index, listener);
            }
            else
            {
                std::string msg("Cannot load file: ");
                msg += Files::pathToUnicodeString(filepath);
                throw std::runtime_error(msg.c_str());
            }
        }

    private:
        std::map<std::string, ContentLoader*> mLoaders;
    };

    struct OMWScriptsLoader : public ContentLoader
    {
        ESMStore& mStore;
        OMWScriptsLoader(ESMStore& store)
            : mStore(store)
        {
        }
        void load(const std::filesystem::path& filepath, int& /*index*/, Loading::Listener* /*listener*/) override
        {
            mStore.addOMWScripts(filepath);
        }
    };

    void World::adjustSky()
    {
        if (mSky && (isCellExterior() || isCellQuasiExterior()))
        {
            mRendering->setSkyEnabled(true);
        }
        else
            mRendering->setSkyEnabled(false);
    }

    World::World(Resource::ResourceSystem* resourceSystem, int activationDistanceOverride, const std::string& startCell,
        const std::filesystem::path& userDataPath)
        : mResourceSystem(resourceSystem)
        , mLocalScripts(mStore)
        , mWorldModel(mStore, mReaders)
        , mTimeManager(std::make_unique<DateTimeManager>())
        , mSky(true)
        , mGodMode(false)
        , mScriptsEnabled(true)
        , mDiscardMovements(true)
        , mUserDataPath(userDataPath)
        , mActivationDistanceOverride(activationDistanceOverride)
        , mStartCell(startCell)
        , mSwimHeightScale(0.f)
        , mDistanceToFacedObject(-1.f)
        , mTeleportEnabled(true)
        , mLevitationEnabled(true)
        , mGoToJail(false)
        , mDaysInPrison(0)
        , mPlayerTraveling(false)
        , mPlayerInJail(false)
        , mSpellPreloadTimer(0.f)
    {
    }

    void World::loadData(const Files::Collections& fileCollections, const std::vector<std::string>& contentFiles,
        const std::vector<std::string>& groundcoverFiles, ToUTF8::Utf8Encoder* encoder, Loading::Listener* listener)
    {
        mContentFiles = contentFiles;
        mESMVersions.resize(mContentFiles.size(), -1);

        loadContentFiles(fileCollections, contentFiles, encoder, listener);
        loadGroundcoverFiles(fileCollections, groundcoverFiles, encoder, listener);

        fillGlobalVariables();

        mStore.setUp();
        mStore.validateRecords(mReaders);
        mStore.movePlayerRecord();

        mSwimHeightScale = mStore.get<ESM::GameSetting>().find("fSwimHeightScale")->mValue.getFloat();
    }

    void World::init(osgViewer::Viewer* viewer, osg::ref_ptr<osg::Group> rootNode, SceneUtil::WorkQueue* workQueue,
        SceneUtil::UnrefQueue& unrefQueue)
    {
        mPhysics = std::make_unique<MWPhysics::PhysicsSystem>(mResourceSystem, rootNode);

        if (Settings::navigator().mEnable)
        {
            auto navigatorSettings = DetourNavigator::makeSettingsFromSettingsManager();
            navigatorSettings.mRecast.mSwimHeightScale = mSwimHeightScale;
            mNavigator = DetourNavigator::makeNavigator(navigatorSettings, mUserDataPath);
        }
        else
        {
            mNavigator = DetourNavigator::makeNavigatorStub();
        }

        mRendering = std::make_unique<MWRender::RenderingManager>(
            viewer, rootNode, mResourceSystem, workQueue, *mNavigator, mGroundcoverStore, unrefQueue);
        mProjectileManager = std::make_unique<ProjectileManager>(
            mRendering->getLightRoot()->asGroup(), mResourceSystem, mRendering.get(), mPhysics.get());
        mRendering->preloadCommonAssets();

        mWeatherManager = std::make_unique<MWWorld::WeatherManager>(*mRendering, mStore);

        mWorldScene = std::make_unique<Scene>(*this, *mRendering.get(), mPhysics.get(), *mNavigator);
    }

    void World::fillGlobalVariables()
    {
        mGlobalVariables.fill(mStore);
        mTimeManager->setup(mGlobalVariables);
    }

    void World::startNewGame(bool bypass)
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
        mRendering->getCamera()->reset();

        // we don't want old weather to persist on a new game
        // Note that if reset later, the initial ChangeWeather that the chargen script calls will be lost.
        mWeatherManager.reset();
        mWeatherManager = std::make_unique<MWWorld::WeatherManager>(*mRendering.get(), mStore);

        if (!bypass)
        {
            // set new game mark
            mGlobalVariables[Globals::sCharGenState].setInteger(1);
        }
        else
            mGlobalVariables[Globals::sCharGenState].setInteger(-1);

        if (bypass && !mStartCell.empty())
        {
            ESM::Position pos;
            ESM::RefId cellId = findExteriorPosition(mStartCell, pos);
            if (!cellId.empty())
            {
                changeToCell(cellId, pos, true);
                adjustPosition(getPlayerPtr(), false);
            }
            else
            {
                findInteriorPosition(mStartCell, pos);
                changeToInteriorCell(mStartCell, pos, true);
            }
        }
        else
        {
            for (int i = 0; i < 5; ++i)
                MWBase::Environment::get().getScriptManager()->getGlobalScripts().run();
            if (!getPlayerPtr().isInCell())
            {
                ESM::Position pos;
                const int cellSize = Constants::CellSizeInUnits;
                pos.pos[0] = cellSize / 2;
                pos.pos[1] = cellSize / 2;
                pos.pos[2] = 0;
                pos.rot[0] = 0;
                pos.rot[1] = 0;
                pos.rot[2] = 0;

                ESM::ExteriorCellLocation exteriorCellPos = ESM::positionToExteriorCellLocation(pos.pos[0], pos.pos[1]);
                ESM::RefId cellId = ESM::RefId::esm3ExteriorCell(exteriorCellPos.mX, exteriorCellPos.mY);
                mWorldScene->changeToExteriorCell(cellId, pos, true);
            }
        }

        if (!bypass)
        {
            std::string_view video = Fallback::Map::getString("Movies_New_Game");
            if (!video.empty())
            {
                // Make sure that we do not continue to play a Title music after a new game video.
                MWBase::Environment::get().getSoundManager()->stopMusic();
                MWBase::Environment::get().getWindowManager()->playVideo(video, true);
            }
        }

        // enable collision
        if (!mPhysics->toggleCollisionMode())
            mPhysics->toggleCollisionMode();

        MWBase::Environment::get().getWindowManager()->updatePlayer();
        mTimeManager->setup(mGlobalVariables);

        // Initial seed.
        mPrng.seed(mRandomSeed);
    }

    void World::clear()
    {
        mWeatherManager->clear();
        mRendering->clear();
        mProjectileManager->clear();
        mLocalScripts.clear();

        mWorldScene->clear();
        mWorldModel.clear();

        mStore.clearDynamic();

        if (mPlayer)
        {
            mPlayer->clear();
            mPlayer->set(mStore.get<ESM::NPC>().find(ESM::RefId::stringRefId("Player")));
        }

        mDoorStates.clear();

        mGoToJail = false;
        mTeleportEnabled = true;
        mLevitationEnabled = true;
        mPlayerTraveling = false;
        mPlayerInJail = false;
        mIdsRebuilt = false;

        fillGlobalVariables();
    }

    int World::countSavedGameRecords() const
    {
        return mWorldModel.countSavedGameRecords() + mStore.countSavedGameRecords()
            + mGlobalVariables.countSavedGameRecords() + mProjectileManager->countSavedGameRecords()
            + 1 // player record
            + 1 // weather record
            + 1 // actorId counter
            + 1 // levitation/teleport enabled state
            + 1 // camera
            + 1; // random state.
    }

    int World::countSavedGameCells() const
    {
        return mWorldModel.countSavedGameRecords();
    }

    void World::write(ESM::ESMWriter& writer, Loading::Listener& progress) const
    {
        writer.startRecord(ESM::REC_RAND);
        writer.writeHNOString("RAND", Misc::Rng::serialize(mPrng));
        writer.endRecord(ESM::REC_RAND);

        // Active cells could have a dirty fog of war, sync it to the CellStore first
        for (CellStore* cellstore : mWorldScene->getActiveCells())
        {
            MWBase::Environment::get().getWindowManager()->writeFog(cellstore);
        }

        MWMechanics::CreatureStats::writeActorIdCounter(writer);

        mStore.write(writer, progress); // dynamic Store must be written (and read) before Cells, so that
                                        // references to custom made records will be recognized
        mWorldModel.write(writer, progress); // the player's cell needs to be loaded before the player
        mPlayer->write(writer, progress);
        mGlobalVariables.write(writer, progress);
        mWeatherManager->write(writer, progress);
        mProjectileManager->write(writer, progress);

        writer.startRecord(ESM::REC_ENAB);
        writer.writeHNT("TELE", mTeleportEnabled);
        writer.writeHNT("LEVT", mLevitationEnabled);
        writer.endRecord(ESM::REC_ENAB);

        writer.startRecord(ESM::REC_CAM_);
        writer.writeHNT("FIRS", isFirstPerson());
        writer.endRecord(ESM::REC_CAM_);
    }

    void World::readRecord(ESM::ESMReader& reader, uint32_t type)
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
            case ESM::REC_RAND:
            {
                auto data = reader.getHNOString("RAND");
                Misc::Rng::deserialize(data, mPrng);
            }
            break;
            case ESM::REC_PLAY:
                if (reader.getFormatVersion() <= ESM::MaxPlayerBeforeCellDataFormatVersion && !mIdsRebuilt)
                {
                    mStore.rebuildIdsIndex();
                    mIdsRebuilt = true;
                }

                mStore.checkPlayer();
                mPlayer->readRecord(reader, type);
                if (getPlayerPtr().isInCell())
                {
                    if (getPlayerPtr().getCell()->isExterior())
                        mWorldScene->preloadTerrain(getPlayerPtr().getRefData().getPosition().asVec3(),
                            getPlayerPtr().getCell()->getCell()->getWorldSpace());
                    mWorldScene->preloadCellWithSurroundings(*getPlayerPtr().getCell());
                }
                break;
            case ESM::REC_CSTA:
                // We need to rebuild the ESMStore index in order to be able to lookup dynamic records while loading the
                // WorldModel and, afterwards, the player.
                if (!mIdsRebuilt)
                {
                    mStore.rebuildIdsIndex();
                    mIdsRebuilt = true;
                }
                mWorldModel.readRecord(reader, type);
                break;
            default:
                if (!mStore.readRecord(reader, type) && !mGlobalVariables.readRecord(reader, type)
                    && !mWeatherManager->readRecord(reader, type) && !mProjectileManager->readRecord(reader, type))
                {
                    throw std::runtime_error("unknown record in saved game");
                }
                break;
        }
    }

    void World::ensureNeededRecords()
    {
        for (const auto& [id, value] : generateDefaultGameSettings())
        {
            if (mStore.get<ESM::GameSetting>().search(id) == nullptr)
            {
                ESM::GameSetting record;
                record.mId = ESM::RefId::stringRefId(id);
                record.mValue = value;
                record.mRecordFlags = 0;
                mStore.insertStatic(record);
            }
        }

        for (const auto& [name, value] : generateDefaultGlobals())
        {
            if (mStore.get<ESM::Global>().search(ESM::RefId::stringRefId(name.getValue())) == nullptr)
            {
                ESM::Global record;
                record.mId = ESM::RefId::stringRefId(name.getValue());
                record.mValue = value;
                record.mRecordFlags = 0;
                mStore.insertStatic(record);
            }
        }

        for (const auto& [id, model] : generateDefaultStatics())
        {
            if (mStore.get<ESM::Static>().search(ESM::RefId::stringRefId(id)) == nullptr)
            {
                ESM::Static record;
                record.mId = ESM::RefId::stringRefId(id);
                record.mModel = model;
                record.mRecordFlags = 0;
                mStore.insertStatic(record);
            }
        }

        for (const auto& [id, model] : generateDefaultDoors())
        {
            if (mStore.get<ESM::Door>().search(ESM::RefId::stringRefId(id)) == nullptr)
            {
                ESM::Door record;
                record.mId = ESM::RefId::stringRefId(id);
                record.mModel = model;
                record.mRecordFlags = 0;
                mStore.insertStatic(record);
            }
        }
    }

    World::~World()
    {
        // Must be cleared before mRendering is destroyed
        if (mProjectileManager)
            mProjectileManager->clear();

        if (Settings::navigator().mWaitForAllJobsOnExit)
        {
            Log(Debug::Verbose) << "Waiting for all navmesh jobs to be done...";
            mNavigator->wait(DetourNavigator::WaitConditionType::allJobsDone, nullptr);
        }
    }

    void World::setRandomSeed(uint32_t seed)
    {
        mRandomSeed = seed;
    }

    void World::useDeathCamera()
    {
        mRendering->getCamera()->setMode(MWRender::Camera::Mode::ThirdPerson);
    }

    MWWorld::Player& World::getPlayer()
    {
        return *mPlayer;
    }

    const std::vector<int>& World::getESMVersions() const
    {
        return mESMVersions;
    }

    LocalScripts& World::getLocalScripts()
    {
        return mLocalScripts;
    }

    void World::setGlobalInt(GlobalVariableName name, int value)
    {
        mTimeManager->updateGlobalInt(name, value);
        mGlobalVariables[name].setInteger(value);
    }

    void World::setGlobalFloat(GlobalVariableName name, float value)
    {
        mTimeManager->updateGlobalFloat(name, value);
        mGlobalVariables[name].setFloat(value);
    }

    int World::getGlobalInt(GlobalVariableName name) const
    {
        return mGlobalVariables[name].getInteger();
    }

    float World::getGlobalFloat(GlobalVariableName name) const
    {
        return mGlobalVariables[name].getFloat();
    }

    char World::getGlobalVariableType(GlobalVariableName name) const
    {
        return mGlobalVariables.getType(name);
    }

    std::string_view World::getCellName(const MWWorld::CellStore* cell) const
    {
        if (!cell)
            cell = mWorldScene->getCurrentCell();
        return getCellName(*cell->getCell());
    }

    std::string_view World::getCellName(const MWWorld::Cell& cell) const
    {
        if (!cell.isExterior() || !cell.getDisplayName().empty())
            return cell.getDisplayName();

        if (!cell.getRegion().empty())
        {
            std::string_view regionName
                = ESM::visit(ESM::VisitOverload{
                                 [&](const ESM::Cell& cellIn) -> std::string_view {
                                     if (const ESM::Region* region = mStore.get<ESM::Region>().search(cell.getRegion()))
                                         return !region->mName.empty() ? region->mName : region->mId.getRefIdString();
                                     return {};
                                 },
                                 [&](const ESM4::Cell& cellIn) -> std::string_view { return {}; },
                             },
                    cell);
            if (!regionName.empty())
                return regionName;
        }

        if (!cell.getWorldSpace().empty() && ESM::isEsm4Ext(cell.getWorldSpace()))
        {
            if (const ESM4::World* worldspace = mStore.get<ESM4::World>().search(cell.getWorldSpace()))
                if (!worldspace->mFullName.empty())
                    return worldspace->mFullName;
        }

        return mStore.get<ESM::GameSetting>().find("sDefaultCellname")->mValue.getString();
    }

    void World::removeRefScript(const MWWorld::CellRef* ref)
    {
        mLocalScripts.remove(ref);
    }

    Ptr World::searchPtr(const ESM::RefId& name, bool activeOnly, bool searchInContainers)
    {
        Ptr ret;
        // the player is always in an active cell.
        if (name == "Player")
        {
            return mPlayer->getPlayer();
        }

        for (CellStore* cellstore : mWorldScene->getActiveCells())
        {
            // TODO: caching still doesn't work efficiently here (only works for the one CellStore that the reference is
            // in)
            Ptr ptr = cellstore->getPtr(name);

            if (!ptr.isEmpty())
                return ptr;
        }

        if (!activeOnly)
        {
            ret = mWorldModel.getPtrByRefId(name);
            if (!ret.isEmpty())
                return ret;
        }

        if (searchInContainers)
        {
            for (CellStore* cellstore : mWorldScene->getActiveCells())
            {
                Ptr ptr = cellstore->searchInContainer(name);
                if (!ptr.isEmpty())
                    return ptr;
            }
        }

        Ptr ptr = mPlayer->getPlayer().getClass().getContainerStore(mPlayer->getPlayer()).search(name);

        return ptr;
    }

    Ptr World::getPtr(const ESM::RefId& name, bool activeOnly)
    {
        Ptr ret = searchPtr(name, activeOnly);
        if (!ret.isEmpty())
            return ret;
        std::string error = "Failed to find an instance of object " + name.toDebugString();
        if (activeOnly)
            error += " in active cells";
        throw std::runtime_error(error);
    }

    Ptr World::searchPtrViaActorId(int actorId)
    {
        // The player is not registered in any CellStore so must be checked manually
        if (actorId == getPlayerPtr().getClass().getCreatureStats(getPlayerPtr()).getActorId())
            return getPlayerPtr();
        // Now search cells
        return mWorldScene->searchPtrViaActorId(actorId);
    }

    struct FindContainerVisitor
    {
        ConstPtr mContainedPtr;
        Ptr mResult;

        FindContainerVisitor(const ConstPtr& containedPtr)
            : mContainedPtr(containedPtr)
        {
        }

        bool operator()(const Ptr& ptr)
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

    void World::addContainerScripts(const Ptr& reference, CellStore* cell)
    {
        if (reference.getType() == ESM::Container::sRecordId || reference.getType() == ESM::NPC::sRecordId
            || reference.getType() == ESM::Creature::sRecordId)
        {
            MWWorld::ContainerStore& container = reference.getClass().getContainerStore(reference);
            for (MWWorld::ContainerStoreIterator it = container.begin(); it != container.end(); ++it)
            {
                const auto& script = it->getClass().getScript(*it);
                if (!script.empty())
                {
                    MWWorld::Ptr item = *it;
                    item.mCell = cell;
                    mLocalScripts.add(script, item);
                }
            }
        }
    }

    void World::enable(const Ptr& reference)
    {
        if (!reference.isInCell())
            return;

        if (!reference.getRefData().isEnabled())
        {
            reference.getRefData().enable();

            if (mWorldScene->getActiveCells().find(reference.getCell()) != mWorldScene->getActiveCells().end()
                && reference.getCellRef().getCount())
                mWorldScene->addObjectToScene(reference);

            if (reference.getCellRef().getRefNum().hasContentFile())
            {
                int type = mStore.find(reference.getCellRef().getRefId());
                if (mRendering->pagingEnableObject(type, reference, true))
                    mWorldScene->reloadTerrain();
            }
        }
    }

    void World::removeContainerScripts(const Ptr& reference)
    {
        if (reference.getType() == ESM::Container::sRecordId || reference.getType() == ESM::NPC::sRecordId
            || reference.getType() == ESM::Creature::sRecordId)
        {
            MWWorld::ContainerStore& container = reference.getClass().getContainerStore(reference);
            for (MWWorld::ContainerStoreIterator it = container.begin(); it != container.end(); ++it)
            {
                const ESM::RefId& script = it->getClass().getScript(*it);
                if (!script.empty())
                {
                    MWWorld::Ptr item = *it;
                    mLocalScripts.remove(item);
                }
            }
        }
    }

    void World::disable(const Ptr& reference)
    {
        if (!reference.getRefData().isEnabled())
            return;

        // disable is a no-op for items in containers
        if (!reference.isInCell())
            return;

        if (reference == getPlayerPtr())
            throw std::runtime_error("can not disable player object");

        reference.getRefData().disable();

        if (reference.getCellRef().getRefNum().hasContentFile())
        {
            int type = mStore.find(reference.getCellRef().getRefId());
            if (mRendering->pagingEnableObject(type, reference, false))
                mWorldScene->reloadTerrain();
        }

        if (mWorldScene->getActiveCells().find(reference.getCell()) != mWorldScene->getActiveCells().end()
            && reference.getCellRef().getCount())
        {
            mWorldScene->removeObjectFromScene(reference);
            mWorldScene->addPostponedPhysicsObjects();
        }
    }

    void World::advanceTime(double hours, bool incremental)
    {
        if (!incremental)
        {
            // When we fast-forward time, we should recharge magic items
            // in all loaded cells, using game world time
            float duration = hours * 3600;
            const float timeScaleFactor = mTimeManager->getGameTimeScale();
            if (timeScaleFactor != 0.0f)
                duration /= timeScaleFactor;

            rechargeItems(duration, false);
        }

        mWeatherManager->advanceTime(hours, incremental);
        mTimeManager->advanceTime(hours, mGlobalVariables);

        if (!incremental)
        {
            mRendering->notifyWorldSpaceChanged();
            mProjectileManager->clear();
            mDiscardMovements = true;
        }
    }

    TimeStamp World::getTimeStamp() const
    {
        return mTimeManager->getTimeStamp();
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

    void World::setMoonColour(bool red)
    {
        mRendering->skySetMoonColour(red);
    }

    void World::changeToInteriorCell(
        const std::string_view cellName, const ESM::Position& position, bool adjustPlayerPos, bool changeEvent)
    {
        mPhysics->clearQueuedMovement();
        mDiscardMovements = true;

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

    void World::changeToCell(
        const ESM::RefId& cellId, const ESM::Position& position, bool adjustPlayerPos, bool changeEvent)
    {
        const MWWorld::Cell* destinationCell = getWorldModel().getCell(cellId).getCell();
        bool exteriorCell = destinationCell->isExterior();

        mPhysics->clearQueuedMovement();
        mDiscardMovements = true;

        if (changeEvent && mCurrentWorldSpace != destinationCell->getNameId())
        {
            // changed worldspace
            mProjectileManager->clear();
            mRendering->notifyWorldSpaceChanged();
            mCurrentWorldSpace = destinationCell->getNameId();
        }
        removeContainerScripts(getPlayerPtr());
        if (exteriorCell)
            mWorldScene->changeToExteriorCell(cellId, position, adjustPlayerPos, changeEvent);
        else
            mWorldScene->changeToInteriorCell(destinationCell->getNameId(), position, adjustPlayerPos, changeEvent);
        addContainerScripts(getPlayerPtr(), getPlayerPtr().getCell());
    }

    float World::getMaxActivationDistance() const
    {
        if (mActivationDistanceOverride >= 0)
            return static_cast<float>(mActivationDistanceOverride);

        static const int iMaxActivateDist
            = mStore.get<ESM::GameSetting>().find("iMaxActivateDist")->mValue.getInteger();
        return static_cast<float>(iMaxActivateDist);
    }

    MWWorld::Ptr World::getFacedObject()
    {
        MWWorld::Ptr facedObject;

        if (MWBase::Environment::get().getStateManager()->getState() == MWBase::StateManager::State_NoGame)
            return facedObject;

        if (MWBase::Environment::get().getWindowManager()->isGuiMode()
            && MWBase::Environment::get().getWindowManager()->isConsoleMode())
            facedObject = getFacedObject(getMaxActivationDistance() * 50, false);
        else
        {
            float activationDistance = getActivationDistancePlusTelekinesis();

            facedObject = getFacedObject(activationDistance, true);

            if (!facedObject.isEmpty() && !facedObject.getClass().allowTelekinesis(facedObject)
                && mDistanceToFacedObject > getMaxActivationDistance()
                && !MWBase::Environment::get().getWindowManager()->isGuiMode())
                return nullptr;
        }
        return facedObject;
    }

    float World::getDistanceToFacedObject()
    {
        return mDistanceToFacedObject;
    }

    osg::Matrixf World::getActorHeadTransform(const MWWorld::ConstPtr& actor) const
    {
        const MWRender::Animation* anim = mRendering->getAnimation(actor);
        if (anim)
        {
            const osg::Node* node = anim->getNode("Head");
            if (!node)
                node = anim->getNode("Bip01 Head");
            if (node)
            {
                osg::NodePathList nodepaths = node->getParentalNodePaths();
                if (!nodepaths.empty())
                    return osg::computeLocalToWorld(nodepaths[0]);
            }
        }
        return osg::Matrixf::translate(actor.getRefData().getPosition().asVec3());
    }

    void World::deleteObject(const Ptr& ptr)
    {
        if (!ptr.mRef->isDeleted() && ptr.getContainerStore() == nullptr)
        {
            if (ptr == getPlayerPtr())
                throw std::runtime_error("can not delete player object");

            ptr.getCellRef().setCount(0);

            if (ptr.isInCell()
                && mWorldScene->getActiveCells().find(ptr.getCell()) != mWorldScene->getActiveCells().end()
                && ptr.getRefData().isEnabled())
            {
                mWorldScene->removeObjectFromScene(ptr);
                mLocalScripts.remove(ptr);
                removeContainerScripts(ptr);
            }
        }
    }

    void World::undeleteObject(const Ptr& ptr)
    {
        if (!ptr.getCellRef().hasContentFile())
            return;
        if (ptr.mRef->isDeleted())
        {
            ptr.getCellRef().setCount(1);
            if (mWorldScene->getActiveCells().find(ptr.getCell()) != mWorldScene->getActiveCells().end()
                && ptr.getRefData().isEnabled())
            {
                mWorldScene->addObjectToScene(ptr);
                const auto& script = ptr.getClass().getScript(ptr);
                if (!script.empty())
                    mLocalScripts.add(script, ptr);
                addContainerScripts(ptr, ptr.getCell());
            }
        }
    }

    MWWorld::Ptr World::moveObject(
        const Ptr& ptr, CellStore* newCell, const osg::Vec3f& position, bool movePhysics, bool keepActive)
    {
        ESM::Position pos = ptr.getRefData().getPosition();
        std::memcpy(pos.pos, &position, sizeof(osg::Vec3f));
        ptr.getRefData().setPosition(pos);

        CellStore* currCell = ptr.isInCell()
            ? ptr.getCell()
            : nullptr; // currCell == nullptr should only happen for player, during initial startup
        bool isPlayer = ptr == mPlayer->getPlayer();
        bool haveToMove = isPlayer || (currCell && mWorldScene->isCellActive(*currCell));
        MWWorld::Ptr newPtr = ptr;

        if (!isPlayer && !currCell)
            throw std::runtime_error("Can not move actor " + ptr.getCellRef().getRefId().toDebugString()
                + " to another cell: current cell is nullptr");

        if (!newCell)
            throw std::runtime_error("Can not move actor " + ptr.getCellRef().getRefId().toDebugString()
                + " to another cell: new cell is nullptr");

        if (currCell != newCell)
        {
            removeContainerScripts(ptr);

            if (isPlayer)
            {
                if (!newCell->isExterior())
                {
                    changeToInteriorCell(newCell->getCell()->getNameId(), pos, false);
                    removeContainerScripts(getPlayerPtr());
                }
                else
                {
                    if (mWorldScene->isCellActive(*newCell))
                        mWorldScene->changePlayerCell(*newCell, pos, false);
                    else
                        mWorldScene->changeToExteriorCell(newCell->getCell()->getId(), pos, false);
                }
                addContainerScripts(getPlayerPtr(), newCell);
                newPtr = getPlayerPtr();
            }
            else
            {
                bool currCellActive = mWorldScene->isCellActive(*currCell);
                bool newCellActive = mWorldScene->isCellActive(*newCell);
                if (!currCellActive && newCellActive)
                {
                    newPtr = currCell->moveTo(ptr, newCell);
                    if (newPtr.getRefData().isEnabled())
                        mWorldScene->addObjectToScene(newPtr);

                    const auto& script = newPtr.getClass().getScript(newPtr);
                    if (!script.empty())
                    {
                        mLocalScripts.add(script, newPtr);
                    }
                    addContainerScripts(newPtr, newCell);
                }
                else if (!newCellActive && currCellActive)
                {
                    mWorldScene->removeObjectFromScene(ptr, keepActive);
                    mLocalScripts.remove(ptr);
                    removeContainerScripts(ptr);
                    haveToMove = false;

                    newPtr = currCell->moveTo(ptr, newCell);
                    newPtr.getRefData().setBaseNode(nullptr);
                }
                else if (!currCellActive && !newCellActive)
                    newPtr = currCell->moveTo(ptr, newCell);
                else // both cells active
                {
                    newPtr = currCell->moveTo(ptr, newCell);

                    mRendering->updatePtr(ptr, newPtr);
                    MWBase::Environment::get().getSoundManager()->updatePtr(ptr, newPtr);
                    mPhysics->updatePtr(ptr, newPtr);

                    MWBase::MechanicsManager* mechMgr = MWBase::Environment::get().getMechanicsManager();
                    mechMgr->updateCell(ptr, newPtr);

                    const auto& script = ptr.getClass().getScript(ptr);
                    if (!script.empty())
                    {
                        mLocalScripts.remove(ptr);
                        removeContainerScripts(ptr);
                        mLocalScripts.add(script, newPtr);
                        addContainerScripts(newPtr, newCell);
                    }
                }
            }

            MWBase::Environment::get().getWindowManager()->updateConsoleObjectPtr(ptr, newPtr);
            MWBase::Environment::get().getScriptManager()->getGlobalScripts().updatePtrs(ptr, newPtr);
        }
        if (haveToMove && newPtr.getRefData().getBaseNode())
        {
            mRendering->moveObject(newPtr, position);
            if (movePhysics)
            {
                mPhysics->updatePosition(newPtr);
                if (const MWPhysics::Object* object = mPhysics->getObject(newPtr))
                    updateNavigatorObject(*object);
            }
        }

        if (isPlayer)
            mWorldScene->playerMoved(position);
        else
        {
            mRendering->pagingBlacklistObject(mStore.find(ptr.getCellRef().getRefId()), ptr);
            mWorldScene->removeFromPagedRefs(newPtr);
        }

        return newPtr;
    }

    MWWorld::Ptr World::moveObject(const Ptr& ptr, const osg::Vec3f& position, bool movePhysics, bool moveToActive)
    {
        CellStore* cell = ptr.getCell();
        ESM::RefId worldspaceId
            = cell->isExterior() ? cell->getCell()->getWorldSpace() : ESM::Cell::sDefaultWorldspaceId;
        const ESM::ExteriorCellLocation index
            = ESM::positionToExteriorCellLocation(position.x(), position.y(), worldspaceId);

        CellStore* newCell = cell->isExterior() ? &mWorldModel.getExterior(index) : nullptr;
        bool isCellActive = getPlayerPtr().isInCell() && getPlayerPtr().getCell()->isExterior()
            && (newCell && mWorldScene->isCellActive(*newCell));

        if (cell->isExterior() || (moveToActive && isCellActive && ptr.getClass().isActor()))
            cell = newCell;

        return moveObject(ptr, cell, position, movePhysics);
    }

    MWWorld::Ptr World::moveObjectBy(const Ptr& ptr, const osg::Vec3f& vec, bool moveToActive)
    {
        auto* actor = mPhysics->getActor(ptr);
        osg::Vec3f newpos = ptr.getRefData().getPosition().asVec3() + vec;
        if (actor)
            actor->adjustPosition(vec);
        if (ptr.getClass().isActor())
            return moveObject(ptr, newpos, false, moveToActive && ptr != getPlayerPtr());
        return moveObject(ptr, newpos);
    }

    void World::scaleObject(const Ptr& ptr, float scale, bool force)
    {
        if (!force && scale == ptr.getCellRef().getScale())
            return;
        if (mPhysics->getActor(ptr))
            mNavigator->removeAgent(getPathfindingAgentBounds(ptr));

        ptr.getCellRef().setScale(scale);
        mRendering->pagingBlacklistObject(mStore.find(ptr.getCellRef().getRefId()), ptr);
        mWorldScene->removeFromPagedRefs(ptr);

        if (ptr.getRefData().getBaseNode() != nullptr)
            mWorldScene->updateObjectScale(ptr);

        if (mPhysics->getActor(ptr))
        {
            const DetourNavigator::AgentBounds agentBounds = getPathfindingAgentBounds(ptr);
            if (!mNavigator->addAgent(agentBounds))
                Log(Debug::Warning) << "Scaled agent bounds are not supported by navigator: " << agentBounds;
        }
        else if (const auto object = mPhysics->getObject(ptr))
            updateNavigatorObject(*object);
    }

    void World::rotateObject(const Ptr& ptr, const osg::Vec3f& rot, MWBase::RotationFlags flags)
    {
        ESM::Position pos = ptr.getRefData().getPosition();
        float* objRot = pos.rot;
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

        if (ptr.getClass().isActor())
        {
            /* HACK? Actors shouldn't really be rotating around X (or Y), but
             * currently it's done so for rotating the camera, which needs
             * clamping.
             */
            objRot[0] = std::clamp<float>(objRot[0], -osg::PI_2, osg::PI_2);
            objRot[1] = Misc::normalizeAngle(objRot[1]);
            objRot[2] = Misc::normalizeAngle(objRot[2]);
        }

        ptr.getRefData().setPosition(pos);

        mRendering->pagingBlacklistObject(mStore.find(ptr.getCellRef().getRefId()), ptr);
        mWorldScene->removeFromPagedRefs(ptr);

        if (ptr.getRefData().getBaseNode() != nullptr)
        {
            const auto order
                = flags & MWBase::RotationFlag_inverseOrder ? RotationOrder::inverse : RotationOrder::direct;
            mWorldScene->updateObjectRotation(ptr, order);

            if (const auto object = mPhysics->getObject(ptr))
                updateNavigatorObject(*object);
        }
    }

    void World::adjustPosition(const Ptr& ptr, bool force)
    {
        if (ptr.isEmpty())
        {
            Log(Debug::Warning) << "Unable to adjust position for empty object";
            return;
        }

        osg::Vec3f pos(ptr.getRefData().getPosition().asVec3());

        if (!ptr.getRefData().getBaseNode())
        {
            // will be adjusted when Ptr's cell becomes active
            return;
        }

        if (!ptr.isInCell())
        {
            Log(Debug::Warning) << "Unable to adjust position for object '" << ptr.getCellRef().getRefId()
                                << "' - it has no cell";
            return;
        }

        const float terrainHeight = ptr.getCell()->isExterior()
            ? getTerrainHeightAt(pos, ptr.getCell()->getCell()->getWorldSpace())
            : -std::numeric_limits<float>::max();
        pos.z() = std::max(pos.z(), terrainHeight)
            + 20; // place slightly above terrain. will snap down to ground with code below

        // We still should trace down dead persistent actors - they do not use the "swimdeath" animation.
        bool swims = ptr.getClass().isActor() && isSwimming(ptr)
            && !(ptr.getClass().isPersistent(ptr) && ptr.getClass().getCreatureStats(ptr).isDeathAnimationFinished());
        if (force || !ptr.getClass().isActor() || (!isFlying(ptr) && !swims && isActorCollisionEnabled(ptr)))
        {
            osg::Vec3f traced
                = mPhysics->traceDown(ptr, pos, ESM::getCellSize(ptr.getCell()->getCell()->getWorldSpace()));
            pos.z() = std::min(pos.z(), traced.z());
        }

        moveObject(ptr, ptr.getCell(), pos);
    }

    void World::fixPosition()
    {
        const MWWorld::Ptr actor = getPlayerPtr();
        const float distance = 128.f;
        ESM::Position esmPos = actor.getRefData().getPosition();
        osg::Quat orientation(esmPos.rot[2], osg::Vec3f(0, 0, -1));
        osg::Vec3f pos(esmPos.asVec3());

        int direction = 0;
        int fallbackDirections[4] = { direction, (direction + 3) % 4, (direction + 2) % 4, (direction + 1) % 4 };

        osg::Vec3f targetPos = pos;
        for (int i = 0; i < 4; ++i)
        {
            direction = fallbackDirections[i];
            if (direction == 0)
                targetPos = pos + (orientation * osg::Vec3f(0, 1, 0)) * distance;
            else if (direction == 1)
                targetPos = pos - (orientation * osg::Vec3f(0, 1, 0)) * distance;
            else if (direction == 2)
                targetPos = pos - (orientation * osg::Vec3f(1, 0, 0)) * distance;
            else if (direction == 3)
                targetPos = pos + (orientation * osg::Vec3f(1, 0, 0)) * distance;

            // destination is free
            if (!mPhysics->castRay(pos, targetPos, MWPhysics::CollisionType_World | MWPhysics::CollisionType_Door).mHit)
                break;
        }
        targetPos.z() += distance / 2.f; // move up a bit to get out from geometry, will snap down later
        osg::Vec3f traced
            = mPhysics->traceDown(actor, targetPos, ESM::getCellSize(actor.getCell()->getCell()->getWorldSpace()));
        if (traced != pos)
        {
            esmPos.pos[0] = traced.x();
            esmPos.pos[1] = traced.y();
            esmPos.pos[2] = traced.z();
            ESM::RefId cell = actor.getCell()->getCell()->getId();
            MWWorld::ActionTeleport(cell, esmPos, false).execute(actor);
        }
    }

    void World::rotateWorldObject(const Ptr& ptr, const osg::Quat& rotate)
    {
        if (ptr.getRefData().getBaseNode() != nullptr)
        {
            mRendering->pagingBlacklistObject(mStore.find(ptr.getCellRef().getRefId()), ptr);
            mWorldScene->removeFromPagedRefs(ptr);

            mRendering->rotateObject(ptr, rotate);
            mPhysics->updateRotation(ptr, rotate);

            if (const auto object = mPhysics->getObject(ptr))
                updateNavigatorObject(*object);
        }
    }

    MWWorld::Ptr World::placeObject(const MWWorld::ConstPtr& ptr, MWWorld::CellStore* cell, const ESM::Position& pos)
    {
        return copyObjectToCell(ptr, cell, pos, ptr.getCellRef().getCount(), false);
    }

    MWWorld::Ptr World::safePlaceObject(const ConstPtr& ptr, const ConstPtr& referenceObject,
        MWWorld::CellStore* referenceCell, int direction, float distance)
    {
        ESM::Position ipos = referenceObject.getRefData().getPosition();
        osg::Vec3f pos(ipos.asVec3());
        osg::Quat orientation(ipos.rot[2], osg::Vec3f(0, 0, -1));

        int fallbackDirections[4] = { direction, (direction + 3) % 4, (direction + 2) % 4, (direction + 1) % 4 };

        osg::Vec3f spawnPoint = pos;

        for (int i = 0; i < 4; ++i)
        {
            direction = fallbackDirections[i];
            if (direction == 0)
                spawnPoint = pos + (orientation * osg::Vec3f(0, 1, 0)) * distance;
            else if (direction == 1)
                spawnPoint = pos - (orientation * osg::Vec3f(0, 1, 0)) * distance;
            else if (direction == 2)
                spawnPoint = pos - (orientation * osg::Vec3f(1, 0, 0)) * distance;
            else if (direction == 3)
                spawnPoint = pos + (orientation * osg::Vec3f(1, 0, 0)) * distance;

            if (!ptr.getClass().isActor())
                break;

            // check if spawn point is safe, fall back to another direction if not
            spawnPoint.z() += 30; // move up a little to account for slopes, will snap down later

            if (!mPhysics
                     ->castRay(spawnPoint, osg::Vec3f(pos.x(), pos.y(), pos.z() + 20),
                         MWPhysics::CollisionType_World | MWPhysics::CollisionType_Door)
                     .mHit)
            {
                // safe
                break;
            }
        }
        ipos.pos[0] = spawnPoint.x();
        ipos.pos[1] = spawnPoint.y();
        ipos.pos[2] = spawnPoint.z();

        if (referenceObject.getClass().isActor())
        {
            ipos.rot[0] = 0;
            ipos.rot[1] = 0;
        }

        MWWorld::Ptr placed = copyObjectToCell(ptr, referenceCell, ipos, ptr.getCellRef().getCount(), false);
        adjustPosition(placed, true); // snap to ground
        return placed;
    }

    void World::queueMovement(const Ptr& ptr, const osg::Vec3f& velocity)
    {
        mPhysics->queueObjectMovement(ptr, velocity);
    }

    void World::updateAnimatedCollisionShape(const Ptr& ptr)
    {
        mPhysics->updateAnimatedCollisionShape(ptr);
    }

    void World::doPhysics(float duration, osg::Timer_t frameStart, unsigned int frameNumber, osg::Stats& stats)
    {
        processDoors(duration);
        mProjectileManager->update(duration);
        mPhysics->stepSimulation(duration, mDiscardMovements, frameStart, frameNumber, stats);
        mProjectileManager->processHits();
        mDiscardMovements = false;
        mPhysics->moveActors();
    }

    void World::updateNavigator()
    {
        auto navigatorUpdateGuard = mNavigator->makeUpdateGuard();

        mPhysics->forEachAnimatedObject([&](const auto& pair) {
            const auto [object, changed] = pair;
            if (changed)
                updateNavigatorObject(*object, navigatorUpdateGuard.get());
        });

        for (const auto& door : mDoorStates)
            if (const auto object = mPhysics->getObject(door.first))
                updateNavigatorObject(*object, navigatorUpdateGuard.get());

        mNavigator->update(getPlayerPtr().getRefData().getPosition().asVec3(), navigatorUpdateGuard.get());
    }

    void World::updateNavigatorObject(
        const MWPhysics::Object& object, const DetourNavigator::UpdateGuard* navigatorUpdateGuard)
    {
        if (object.getShapeInstance()->mVisualCollisionType != Resource::VisualCollisionType::None)
            return;
        const MWWorld::Ptr ptr = object.getPtr();
        const DetourNavigator::ObjectShapes shapes(object.getShapeInstance(),
            DetourNavigator::ObjectTransform{ ptr.getRefData().getPosition(), ptr.getCellRef().getScale() });
        mNavigator->updateObject(
            DetourNavigator::ObjectId(&object), shapes, object.getTransform(), navigatorUpdateGuard);
    }

    const MWPhysics::RayCastingInterface* World::getRayCasting() const
    {
        return mPhysics.get();
    }

    bool World::rotateDoor(const Ptr door, MWWorld::DoorState state, float duration)
    {
        const ESM::Position& objPos = door.getRefData().getPosition();
        auto oldRot = objPos.asRotationVec3();
        auto newRot = oldRot;

        float minRot = door.getCellRef().getPosition().rot[2];
        float maxRot = minRot + osg::DegreesToRadians(90.f);

        float diff = duration * osg::DegreesToRadians(90.f) * (state == MWWorld::DoorState::Opening ? 1 : -1);
        float targetRot = std::clamp(oldRot.z() + diff, minRot, maxRot);
        newRot.z() = targetRot;
        rotateObject(door, newRot, MWBase::RotationFlag_none);

        bool reached = (targetRot == maxRot && state != MWWorld::DoorState::Idle) || targetRot == minRot;

        /// \todo should use convexSweepTest here
        bool collisionWithActor = false;
        for (auto& [ptr, point, normal] :
            mPhysics->getCollisionsPoints(door, MWPhysics::CollisionType_Door, MWPhysics::CollisionType_Actor))
        {

            if (ptr.getClass().isActor())
            {
                auto localPoint = objPos.asVec3() - point;
                osg::Vec3f direction = osg::Quat(diff, osg::Vec3f(0, 0, 1)) * localPoint - localPoint;
                direction.normalize();
                mPhysics->reportCollision(Misc::Convert::toBullet(point), Misc::Convert::toBullet(normal));
                if (direction * normal < 0) // door is turning away from actor
                    continue;

                collisionWithActor = true;

                // Collided with actor, ask actor to try to avoid door
                if (ptr != getPlayerPtr())
                {
                    MWMechanics::AiSequence& seq = ptr.getClass().getCreatureStats(ptr).getAiSequence();
                    if (seq.getTypeId() != MWMechanics::AiPackageTypeId::AvoidDoor) // Only add it once
                        seq.stack(MWMechanics::AiAvoidDoor(door), ptr);
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
                const ESM::RefId& openSound = ref->mOpenSound;
                if (!openSound.empty()
                    && MWBase::Environment::get().getSoundManager()->getSoundPlaying(door, openSound))
                    MWBase::Environment::get().getSoundManager()->stopSound3D(door, openSound);
            }
            else if (state == MWWorld::DoorState::Closing)
            {
                const ESM::RefId& closeSound = ref->mCloseSound;
                if (!closeSound.empty()
                    && MWBase::Environment::get().getSoundManager()->getSoundPlaying(door, closeSound))
                    MWBase::Environment::get().getSoundManager()->stopSound3D(door, closeSound);
            }

            rotateObject(door, oldRot, MWBase::RotationFlag_none);
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
                // Once we load the door's cell again (or re-enable the door), Door::insertObject will reinsert to
                // mDoorStates.
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
        MWPhysics::Actor* physicActor = mPhysics->getActor(ptr);
        if (physicActor && physicActor->getCollisionMode() != internal)
        {
            physicActor->enableCollisionMode(internal);
            physicActor->enableCollisionBody(external);
        }
    }

    bool World::isActorCollisionEnabled(const MWWorld::Ptr& ptr)
    {
        MWPhysics::Actor* physicActor = mPhysics->getActor(ptr);
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

    bool World::toggleRenderMode(MWRender::RenderMode mode)
    {
        switch (mode)
        {
            case MWRender::Render_CollisionDebug:
                return mPhysics->toggleDebugRendering();
            default:
                return mRendering->toggleRenderMode(mode);
        }
    }

    void World::update(float duration, bool paused)
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

        updateNavigator();

        mPlayer->update();

        mPhysics->debugDraw();

        mWorldScene->update(duration);

        mRendering->update(duration, paused);

        updateSoundListener();

        mSpellPreloadTimer -= duration;
        if (mSpellPreloadTimer <= 0.f)
        {
            mSpellPreloadTimer = 0.1f;
            preloadSpells();
        }

        if (mWorldScene->hasCellLoaded())
        {
            mNavigator->wait(DetourNavigator::WaitConditionType::requiredTilesPresent,
                MWBase::Environment::get().getWindowManager()->getLoadingScreen());
            mWorldScene->resetCellLoaded();
        }
    }

    void World::updatePhysics(
        float duration, bool paused, osg::Timer_t frameStart, unsigned int frameNumber, osg::Stats& stats)
    {
        if (!paused)
        {
            doPhysics(duration, frameStart, frameNumber, stats);
        }
        else
        {
            // zero the async stats if we are paused
            stats.setAttribute(frameNumber, "physicsworker_time_begin", 0);
            stats.setAttribute(frameNumber, "physicsworker_time_taken", 0);
            stats.setAttribute(frameNumber, "physicsworker_time_end", 0);
        }
    }

    void World::preloadSpells()
    {
        const ESM::RefId& selectedSpell = MWBase::Environment::get().getWindowManager()->getSelectedSpell();
        if (!selectedSpell.empty())
        {
            const ESM::Spell* spell = mStore.get<ESM::Spell>().search(selectedSpell);
            if (spell)
                preloadEffects(&spell->mEffects);
        }
        const MWWorld::Ptr& selectedEnchantItem
            = MWBase::Environment::get().getWindowManager()->getSelectedEnchantItem();
        if (!selectedEnchantItem.isEmpty())
        {
            const ESM::RefId& enchantId = selectedEnchantItem.getClass().getEnchantment(selectedEnchantItem);
            if (!enchantId.empty())
            {
                const ESM::Enchantment* ench = mStore.get<ESM::Enchantment>().search(enchantId);
                if (ench)
                    preloadEffects(&ench->mEffects);
            }
        }
        const MWWorld::Ptr& selectedWeapon = MWBase::Environment::get().getWindowManager()->getSelectedWeapon();
        if (!selectedWeapon.isEmpty())
        {
            const ESM::RefId& enchantId = selectedWeapon.getClass().getEnchantment(selectedWeapon);
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
        const MWRender::Camera* camera = mRendering->getCamera();
        const auto& player = getPlayerPtr();
        const ESM::Position& refpos = player.getRefData().getPosition();
        osg::Vec3f listenerPos, up, forward;
        osg::Quat listenerOrient;

        if (isFirstPerson() || Settings::sound().mCameraListener)
            listenerPos = camera->getPosition();
        else
            listenerPos = refpos.asVec3() + osg::Vec3f(0, 0, 1.85f * mPhysics->getHalfExtents(player).z());

        if (isFirstPerson() || Settings::sound().mCameraListener)
            listenerOrient = camera->getOrient();
        else
            listenerOrient = osg::Quat(refpos.rot[1], osg::Vec3f(0, -1, 0))
                * osg::Quat(refpos.rot[0], osg::Vec3f(-1, 0, 0)) * osg::Quat(refpos.rot[2], osg::Vec3f(0, 0, -1));

        forward = listenerOrient * osg::Vec3f(0, 1, 0);
        up = listenerOrient * osg::Vec3f(0, 0, 1);

        bool underwater = isUnderwater(player.getCell(), camera->getPosition());

        MWBase::Environment::get().getSoundManager()->setListenerPosDir(listenerPos, forward, up, underwater);
    }

    void World::updateWindowManager()
    {
        try
        {
            // inform the GUI about focused object
            MWWorld::Ptr object = getFacedObject();

            // retrieve object dimensions so we know where to place the floating label
            if (!object.isEmpty())
            {
                osg::BoundingBox bb = mPhysics->getBoundingBox(object);
                if (!bb.valid() && object.getRefData().getBaseNode())
                {
                    osg::ComputeBoundsVisitor computeBoundsVisitor;
                    computeBoundsVisitor.setTraversalMask(~(MWRender::Mask_ParticleSystem | MWRender::Mask_Effect));
                    object.getRefData().getBaseNode()->accept(computeBoundsVisitor);
                    bb = computeBoundsVisitor.getBoundingBox();
                }
                osg::Vec4f screenBounds = mRendering->getScreenBounds(bb);
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
        const float camDist = mRendering->getCamera()->getCameraDistance();
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
        if (facedObject.isEmpty() && rayToObject.mHitRefnum.isSet())
            facedObject = MWBase::Environment::get().getWorldModel()->getPtr(rayToObject.mHitRefnum);
        if (rayToObject.mHit)
            mDistanceToFacedObject = (rayToObject.mRatio * maxDistance) - camDist;
        else
            mDistanceToFacedObject = -1;
        return facedObject;
    }

    bool World::castRenderingRay(MWPhysics::RayCastingResult& res, const osg::Vec3f& from, const osg::Vec3f& to,
        bool ignorePlayer, bool ignoreActors, std::span<const MWWorld::Ptr> ignoreList)
    {
        MWRender::RenderingManager::RayResult rayRes
            = mRendering->castRay(from, to, ignorePlayer, ignoreActors, ignoreList);
        res.mHit = rayRes.mHit;
        res.mHitPos = rayRes.mHitPointWorld;
        res.mHitNormal = rayRes.mHitNormalWorld;
        res.mHitObject = rayRes.mHitObject;
        if (res.mHitObject.isEmpty() && rayRes.mHitRefnum.isSet())
            res.mHitObject = MWBase::Environment::get().getWorldModel()->getPtr(rayRes.mHitRefnum);
        return res.mHit;
    }

    bool World::isCellExterior() const
    {
        const CellStore* currentCell = mWorldScene->getCurrentCell();
        if (currentCell)
        {
            return currentCell->getCell()->isExterior();
        }
        return false;
    }

    bool World::isCellQuasiExterior() const
    {
        const CellStore* currentCell = mWorldScene->getCurrentCell();
        if (currentCell)
        {
            return currentCell->getCell()->isQuasiExterior();
        }
        return false;
    }

    ESM::RefId World::getCurrentWorldspace() const
    {
        const CellStore* cellStore = mWorldScene->getCurrentCell();
        if (cellStore)
            return cellStore->getCell()->getWorldSpace();
        return ESM::Cell::sDefaultWorldspaceId;
    }

    int World::getCurrentWeather() const
    {
        return mWeatherManager->getWeatherID();
    }

    int World::getNextWeather() const
    {
        return mWeatherManager->getNextWeatherID();
    }

    float World::getWeatherTransition() const
    {
        return mWeatherManager->getTransitionFactor();
    }

    unsigned int World::getNightDayMode() const
    {
        return mWeatherManager->getNightDayMode();
    }

    void World::changeWeather(const ESM::RefId& region, const unsigned int id)
    {
        mWeatherManager->changeWeather(region, id);
    }

    void World::modRegion(const ESM::RefId& regionid, const std::vector<uint8_t>& chances)
    {
        mWeatherManager->modRegion(regionid, chances);
    }

    struct GetDoorMarkerVisitor
    {
        std::vector<World::DoorMarker>& mOut;

        bool operator()(const MWWorld::Ptr& ptr)
        {
            MWWorld::LiveCellRef<ESM::Door>& ref = *static_cast<MWWorld::LiveCellRef<ESM::Door>*>(ptr.getBase());

            if (!ref.mData.isEnabled() || ref.isDeleted())
                return true;

            if (ref.mRef.getTeleport())
            {
                World::DoorMarker newMarker;
                newMarker.name = MWClass::Door::getDestination(ref);
                newMarker.dest = ref.mRef.getDestCell();

                ESM::Position pos = ref.mData.getPosition();

                newMarker.x = pos.pos[0];
                newMarker.y = pos.pos[1];
                mOut.push_back(newMarker);
            }
            return true;
        }
    };

    void World::getDoorMarkers(CellStore& cell, std::vector<World::DoorMarker>& out)
    {
        GetDoorMarkerVisitor visitor{ out };
        cell.forEachType<ESM::Door>(visitor);
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

    void World::PCDropped(const Ptr& item)
    {
        const auto& script = item.getClass().getScript(item);

        // Set OnPCDrop Variable on item's script, if it has a script with that variable declared
        if (!script.empty())
            item.getRefData().getLocals().setVarByInt(script, "onpcdrop", 1);
    }

    MWWorld::Ptr World::placeObject(const MWWorld::Ptr& object, float cursorX, float cursorY, int amount, bool copy)
    {
        const float maxDist = 200.f;

        MWRender::RenderingManager::RayResult result
            = mRendering->castCameraToViewportRay(cursorX, cursorY, maxDist, true, true);

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
        Ptr dropped
            = copy ? copyObjectToCell(object, cell, pos, amount, true) : moveObjectToCell(object, cell, pos, true);

        // only the player place items in the world, so no need to check actor
        PCDropped(dropped);

        return dropped;
    }

    bool World::canPlaceObject(float cursorX, float cursorY)
    {
        const float maxDist = 200.f;
        MWRender::RenderingManager::RayResult result
            = mRendering->castCameraToViewportRay(cursorX, cursorY, maxDist, true, true);

        if (result.mHit)
        {
            // check if the wanted position is on a flat surface, and not e.g. against a vertical wall
            if (std::acos((result.mHitNormalWorld / result.mHitNormalWorld.length()) * osg::Vec3f(0, 0, 1))
                >= osg::DegreesToRadians(30.f))
                return false;

            return true;
        }
        else
            return false;
    }

    Ptr World::copyObjectToCell(const ConstPtr& object, CellStore* cell, ESM::Position pos, int count, bool adjustPos)
    {
        if (!cell)
            throw std::runtime_error("copyObjectToCell(): cannot copy object to null cell");
        if (cell->isExterior())
        {
            const ESM::ExteriorCellLocation index
                = ESM::positionToExteriorCellLocation(pos.pos[0], pos.pos[1], cell->getCell()->getWorldSpace());
            cell = &mWorldModel.getExterior(index);
        }

        MWWorld::Ptr dropped = object.getClass().copyToCell(object, *cell, pos, count);

        initObjectInCell(dropped, *cell, adjustPos);

        return dropped;
    }

    Ptr World::moveObjectToCell(const Ptr& object, CellStore* cell, ESM::Position pos, bool adjustPos)
    {
        if (!cell)
            throw std::runtime_error("moveObjectToCell(): cannot move object to null cell");
        if (cell->isExterior())
        {
            const ESM::ExteriorCellLocation index
                = ESM::positionToExteriorCellLocation(pos.pos[0], pos.pos[1], cell->getCell()->getWorldSpace());
            cell = &mWorldModel.getExterior(index);
        }

        MWWorld::Ptr dropped = object.getClass().moveToCell(object, *cell, pos);

        initObjectInCell(dropped, *cell, adjustPos);

        return dropped;
    }

    void World::initObjectInCell(const Ptr& object, CellStore& cell, bool adjustPos)
    {
        if (mWorldScene->isCellActive(cell))
        {
            if (object.getRefData().isEnabled())
            {
                mWorldScene->addObjectToScene(object);
            }
            const auto& script = object.getClass().getScript(object);
            if (!script.empty())
            {
                mLocalScripts.add(script, object);
            }
            addContainerScripts(object, &cell);
        }

        if (!object.getClass().isActor() && adjustPos && object.getRefData().getBaseNode())
        {
            // Adjust position so the location we wanted ends up in the middle of the object bounding box
            osg::ComputeBoundsVisitor computeBounds;
            computeBounds.setTraversalMask(~MWRender::Mask_ParticleSystem);
            object.getRefData().getBaseNode()->accept(computeBounds);
            osg::BoundingBox bounds = computeBounds.getBoundingBox();
            if (bounds.valid())
            {
                ESM::Position pos = object.getRefData().getPosition();
                bounds.set(bounds._min - pos.asVec3(), bounds._max - pos.asVec3());

                osg::Vec3f adjust(
                    (bounds.xMin() + bounds.xMax()) / 2, (bounds.yMin() + bounds.yMax()) / 2, bounds.zMin());
                pos.pos[0] -= adjust.x();
                pos.pos[1] -= adjust.y();
                pos.pos[2] -= adjust.z();
                moveObject(object, pos.asVec3());
            }
        }
    }

    MWWorld::Ptr World::dropObjectOnGround(const Ptr& actor, const Ptr& object, int amount, bool copy)
    {
        MWWorld::CellStore* cell = actor.getCell();

        ESM::Position pos = actor.getRefData().getPosition();
        // We want only the Z part of the actor's rotation
        pos.rot[0] = 0;
        pos.rot[1] = 0;

        osg::Vec3f orig = pos.asVec3();
        orig.z() += 20;
        osg::Vec3f dir(0, 0, -1);

        float len = 1000000.0;

        MWRender::RenderingManager::RayResult result = mRendering->castRay(orig, orig + dir * len, true, true);
        if (result.mHit)
            pos.pos[2] = result.mHitPointWorld.z();

        // copy the object and set its count
        Ptr dropped
            = copy ? copyObjectToCell(object, cell, pos, amount, true) : moveObjectToCell(object, cell, pos, true);

        if (actor == mPlayer->getPlayer()) // Only call if dropped by player
            PCDropped(dropped);
        return dropped;
    }

    void World::processChangedSettings(const Settings::CategorySettingVector& settings)
    {
        mRendering->processChangedSettings(settings);
    }

    bool World::isFlying(const MWWorld::Ptr& ptr) const
    {
        if (!ptr.getClass().isActor())
            return false;

        const MWMechanics::CreatureStats& stats = ptr.getClass().getCreatureStats(ptr);

        if (stats.isDead())
            return false;

        const bool isPlayer = ptr == getPlayerConstPtr();
        if (!(isPlayer && mGodMode)
            && stats.getMagicEffects().getOrDefault(ESM::MagicEffect::Paralyze).getModifier() > 0)
            return false;

        if (ptr.getClass().canFly(ptr))
            return true;

        if (stats.getMagicEffects().getOrDefault(ESM::MagicEffect::Levitate).getMagnitude() > 0
            && isLevitationEnabled())
            return true;

        const MWPhysics::Actor* actor = mPhysics->getActor(ptr);
        if (!actor)
            return true;

        return false;
    }

    bool World::isSlowFalling(const MWWorld::Ptr& ptr) const
    {
        if (!ptr.getClass().isActor())
            return false;

        const MWMechanics::CreatureStats& stats = ptr.getClass().getCreatureStats(ptr);
        if (stats.getMagicEffects().getOrDefault(ESM::MagicEffect::SlowFall).getMagnitude() > 0)
            return true;

        return false;
    }

    bool World::isSubmerged(const MWWorld::ConstPtr& object) const
    {
        return isUnderwater(object, 1.0f / mSwimHeightScale);
    }

    bool World::isSwimming(const MWWorld::ConstPtr& object) const
    {
        return isUnderwater(object, mSwimHeightScale);
    }

    bool World::isWading(const MWWorld::ConstPtr& object) const
    {
        const float kneeDeep = 0.25f;
        return isUnderwater(object, kneeDeep);
    }

    bool World::isUnderwater(const MWWorld::ConstPtr& object, const float heightRatio) const
    {
        osg::Vec3f pos(object.getRefData().getPosition().asVec3());

        pos.z() += heightRatio * 2 * mPhysics->getRenderingHalfExtents(object).z();

        const CellStore* currCell = object.isInCell()
            ? object.getCell()
            : nullptr; // currCell == nullptr should only happen for player, during initial startup

        return isUnderwater(currCell, pos);
    }

    bool World::isUnderwater(const MWWorld::CellStore* cell, const osg::Vec3f& pos) const
    {
        if (!cell)
            return false;

        if (!(cell->getCell()->hasWater()))
        {
            return false;
        }
        return pos.z() < cell->getWaterLevel();
    }

    bool World::isWaterWalkingCastableOnTarget(const MWWorld::ConstPtr& target) const
    {
        const MWWorld::CellStore* cell = target.getCell();
        if (!cell->getCell()->hasWater())
            return true;

        float waterlevel = cell->getWaterLevel();

        // SwimHeightScale affects the upper z position an actor can swim to
        // while in water. Based on observation from the original engine,
        // the upper z position you get with a +1 SwimHeightScale is the depth
        // limit for being able to cast water walking on an underwater target.
        if (isUnderwater(target, mSwimHeightScale + 1)
            || (isUnderwater(cell, target.getRefData().getPosition().asVec3())
                && !mPhysics->canMoveToWaterSurface(target, waterlevel)))
            return false; // not castable if too deep or if not enough room to move actor to surface
        else
            return true;
    }

    bool World::isOnGround(const MWWorld::Ptr& ptr) const
    {
        return mPhysics->isOnGround(ptr);
    }

    void World::togglePOV(bool force)
    {
        mRendering->getCamera()->toggleViewMode(force);
    }

    bool World::isFirstPerson() const
    {
        return mRendering->getCamera()->getMode() == MWRender::Camera::Mode::FirstPerson;
    }

    bool World::isPreviewModeEnabled() const
    {
        return mRendering->getCamera()->getMode() == MWRender::Camera::Mode::Preview;
    }

    bool World::toggleVanityMode(bool enable)
    {
        return mRendering->getCamera()->toggleVanityMode(enable);
    }

    void World::disableDeferredPreviewRotation()
    {
        mRendering->getCamera()->disableDeferredPreviewRotation();
    }

    void World::applyDeferredPreviewRotationToPlayer(float dt)
    {
        mRendering->getCamera()->applyDeferredPreviewRotationToPlayer(dt);
    }

    MWRender::Camera* World::getCamera()
    {
        return mRendering->getCamera();
    }

    bool World::vanityRotateCamera(const float* rot)
    {
        auto* camera = mRendering->getCamera();
        if (!camera->isVanityOrPreviewModeEnabled())
            return false;

        camera->setPitch(camera->getPitch() + rot[0]);
        camera->setYaw(camera->getYaw() + rot[2]);
        return true;
    }

    void World::saveLoaded()
    {
        mStore.rebuildIdsIndex();
        mStore.validateDynamic();
        mTimeManager->setup(mGlobalVariables);
    }

    void World::setupPlayer()
    {
        const ESM::NPC* player = mStore.get<ESM::NPC>().find(ESM::RefId::stringRefId("Player"));
        if (!mPlayer)
            mPlayer = std::make_unique<MWWorld::Player>(player);
        else
        {
            // Remove the old CharacterController
            MWBase::Environment::get().getMechanicsManager()->remove(getPlayerPtr(), true);
            mNavigator->removeAgent(getPathfindingAgentBounds(getPlayerConstPtr()));
            mPhysics->remove(getPlayerPtr());
            mRendering->removePlayer(getPlayerPtr());
            MWBase::Environment::get().getLuaManager()->objectRemovedFromScene(getPlayerPtr());

            mPlayer->set(player);
        }

        Ptr ptr = mPlayer->getPlayer();
        mRendering->setupPlayer(ptr);
        MWBase::Environment::get().getLuaManager()->setupPlayer(ptr);
    }

    void World::renderPlayer()
    {
        MWBase::Environment::get().getMechanicsManager()->remove(getPlayerPtr(), true);

        MWWorld::Ptr player = getPlayerPtr();

        mRendering->renderPlayer(player);
        MWRender::NpcAnimation* anim = static_cast<MWRender::NpcAnimation*>(mRendering->getAnimation(player));
        player.getClass().getInventoryStore(player).setInvListener(anim);
        player.getClass().getInventoryStore(player).setContListener(anim);

        scaleObject(player, player.getCellRef().getScale(), true); // apply race height
        rotateObject(player, osg::Vec3f(), MWBase::RotationFlag_inverseOrder | MWBase::RotationFlag_adjust);

        MWBase::Environment::get().getMechanicsManager()->add(getPlayerPtr());
        MWBase::Environment::get().getWindowManager()->watchActor(getPlayerPtr());

        mPhysics->remove(getPlayerPtr());
        mPhysics->addActor(
            getPlayerPtr(), VFS::Path::toNormalized(getPlayerPtr().getClass().getCorrectedModel(getPlayerPtr())));

        applyLoopingParticles(player);

        const DetourNavigator::AgentBounds agentBounds = getPathfindingAgentBounds(getPlayerConstPtr());
        if (!mNavigator->addAgent(agentBounds))
            Log(Debug::Warning) << "Player agent bounds are not supported by navigator: " << agentBounds;
    }

    World::RestPermitted World::canRest() const
    {
        CellStore* currentCell = mWorldScene->getCurrentCell();

        Ptr player = mPlayer->getPlayer();
        RefData& refdata = player.getRefData();
        osg::Vec3f playerPos(refdata.getPosition().asVec3());

        const MWPhysics::Actor* actor = mPhysics->getActor(player);
        if (!actor)
            throw std::runtime_error("can't find player");

        if (mPlayer->enemiesNearby())
            return Rest_EnemiesAreNearby;

        if (isUnderwater(currentCell, playerPos) || isWalkingOnWater(player))
            return Rest_PlayerIsUnderwater;

        float fallHeight = player.getClass().getCreatureStats(player).getFallHeight();
        float epsilon = 1e-4;
        if ((actor->getCollisionMode() && (!mPhysics->isOnSolidGround(player) || fallHeight >= epsilon))
            || isFlying(player))
            return Rest_PlayerIsInAir;

        if (currentCell->getCell()->noSleep() || player.getClass().getNpcStats(player).isWerewolf())
            return Rest_OnlyWaiting;

        return Rest_Allowed;
    }

    MWRender::Animation* World::getAnimation(const MWWorld::Ptr& ptr)
    {
        auto* animation = mRendering->getAnimation(ptr);
        if (!animation)
        {
            mWorldScene->removeFromPagedRefs(ptr);
            animation = mRendering->getAnimation(ptr);
            if (animation)
                mRendering->pagingBlacklistObject(mStore.find(ptr.getCellRef().getRefId()), ptr);
        }
        return animation;
    }

    const MWRender::Animation* World::getAnimation(const MWWorld::ConstPtr& ptr) const
    {
        return mRendering->getAnimation(ptr);
    }

    void World::screenshot(osg::Image* image, int w, int h)
    {
        mRendering->screenshot(image, w, h);
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

    void World::activateDoor(const Ptr& door, MWWorld::DoorState state)
    {
        door.getClass().setDoorState(door, state);
        mDoorStates[door] = state;
        if (state == MWWorld::DoorState::Idle)
        {
            mDoorStates.erase(door);
            rotateDoor(door, state, 1);
        }
    }

    bool World::getPlayerStandingOn(const MWWorld::ConstPtr& object)
    {
        MWWorld::Ptr player = getPlayerPtr();
        return mPhysics->isActorStandingOn(player, object);
    }

    bool World::getActorStandingOn(const MWWorld::ConstPtr& object)
    {
        std::vector<MWWorld::Ptr> actors;
        mPhysics->getActorsStandingOn(object, actors);
        return !actors.empty();
    }

    void World::getActorsStandingOn(const MWWorld::ConstPtr& object, std::vector<MWWorld::Ptr>& actors)
    {
        mPhysics->getActorsStandingOn(object, actors);
    }

    bool World::getPlayerCollidingWith(const MWWorld::ConstPtr& object)
    {
        return mPhysics->isObjectCollidingWith(object, MWPhysics::ScriptedCollisionType_Player);
    }

    bool World::getActorCollidingWith(const MWWorld::ConstPtr& object)
    {
        return mPhysics->isObjectCollidingWith(object, MWPhysics::ScriptedCollisionType_Actor);
    }

    void World::hurtStandingActors(const ConstPtr& object, float healthPerSecond)
    {
        if (MWBase::Environment::get().getWindowManager()->isGuiMode())
            return;

        std::vector<MWWorld::Ptr> actors;
        mPhysics->getActorsStandingOn(object, actors);
        for (const Ptr& actor : actors)
        {
            MWMechanics::CreatureStats& stats = actor.getClass().getCreatureStats(actor);
            if (stats.isDead())
                continue;

            mPhysics->markAsNonSolid(object);

            if (actor == getPlayerPtr() && mGodMode)
                continue;

            MWMechanics::DynamicStat<float> health = stats.getHealth();
            health.setCurrent(health.getCurrent() - healthPerSecond * MWBase::Environment::get().getFrameDuration());
            stats.setHealth(health);

            if (healthPerSecond > 0.0f)
            {
                if (actor == getPlayerPtr())
                    MWBase::Environment::get().getWindowManager()->activateHitOverlay(false);

                auto healthDamage = ESM::RefId::stringRefId("Health Damage");
                if (!MWBase::Environment::get().getSoundManager()->getSoundPlaying(actor, healthDamage))
                    MWBase::Environment::get().getSoundManager()->playSound3D(actor, healthDamage, 1.0f, 1.0f);
            }
        }
    }

    void World::hurtCollidingActors(const ConstPtr& object, float healthPerSecond)
    {
        if (MWBase::Environment::get().getWindowManager()->isGuiMode())
            return;

        std::vector<Ptr> actors;
        mPhysics->getActorsCollidingWith(object, actors);
        for (const Ptr& actor : actors)
        {
            MWMechanics::CreatureStats& stats = actor.getClass().getCreatureStats(actor);
            if (stats.isDead())
                continue;

            mPhysics->markAsNonSolid(object);

            if (actor == getPlayerPtr() && mGodMode)
                continue;

            MWMechanics::DynamicStat<float> health = stats.getHealth();
            health.setCurrent(health.getCurrent() - healthPerSecond * MWBase::Environment::get().getFrameDuration());
            stats.setHealth(health);

            if (healthPerSecond > 0.0f)
            {
                if (actor == getPlayerPtr())
                    MWBase::Environment::get().getWindowManager()->activateHitOverlay(false);

                auto healthDamage = ESM::RefId::stringRefId("Health Damage");
                if (!MWBase::Environment::get().getSoundManager()->getSoundPlaying(actor, healthDamage))
                    MWBase::Environment::get().getSoundManager()->playSound3D(actor, healthDamage, 1.0f, 1.0f);
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
            return osg::Vec3f(0, 1, 0);
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
            if (ptr.mRef->isDeleted())
                return true;

            // vanilla Morrowind does not allow to sell items from containers with zero capacity
            if (ptr.getClass().getCapacity(ptr) <= 0.f)
                return true;

            if (ptr.getCellRef().getOwner() == mOwner.getCellRef().getRefId())
                mOut.push_back(ptr);

            return true;
        }
    };

    void World::getContainersOwnedBy(const MWWorld::ConstPtr& owner, std::vector<MWWorld::Ptr>& out)
    {
        for (CellStore* cellstore : mWorldScene->getActiveCells())
        {
            GetContainersOwnedByVisitor visitor(owner, out);
            cellstore->forEachType<ESM::Container>(visitor);
        }
    }

    void World::getItemsOwnedBy(const MWWorld::ConstPtr& npc, std::vector<MWWorld::Ptr>& out)
    {
        for (CellStore* cellstore : mWorldScene->getActiveCells())
        {
            cellstore->forEach([&](const auto& ptr) {
                if (ptr.getRefData().getBaseNode() && ptr.getCellRef().getOwner() == npc.getCellRef().getRefId())
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
        osg::Vec3f to(dir);
        to.normalize();
        to = from + (to * maxDist);

        int collisionTypes
            = MWPhysics::CollisionType_World | MWPhysics::CollisionType_HeightMap | MWPhysics::CollisionType_Door;
        if (includeWater)
        {
            collisionTypes |= MWPhysics::CollisionType_Water;
        }
        MWPhysics::RayCastingResult result
            = mPhysics->castRay(from, to, { MWWorld::Ptr() }, std::vector<MWWorld::Ptr>(), collisionTypes);

        if (!result.mHit)
            return maxDist;
        else
            return (result.mHitPos - from).length();
    }

    void World::enableActorCollision(const MWWorld::Ptr& actor, bool enable)
    {
        MWPhysics::Actor* physicActor = mPhysics->getActor(actor);
        if (physicActor)
            physicActor->enableCollisionBody(enable);
    }

    static std::optional<ESM::Position> searchMarkerPosition(const CellStore& cellStore, std::string_view editorId)
    {
        for (const MWWorld::LiveCellRef<ESM4::Static>& stat4 : cellStore.getReadOnlyEsm4Statics().mList)
        {
            if (Misc::StringUtils::lowerCase(stat4.mBase->mEditorId) == editorId)
                return stat4.mRef.getPosition();
        }
        return std::nullopt;
    }

    static std::optional<ESM::Position> searchDoorDestInCell(const CellStore& cellStore)
    {
        ESM::RefId cellId = cellStore.getCell()->getId();
        std::vector<const MWWorld::CellRef*> sortedDoors;
        for (const MWWorld::LiveCellRef<ESM::Door>& door : cellStore.getReadOnlyDoors().mList)
        {
            if (!door.mRef.getTeleport())
                continue;
            sortedDoors.push_back(&door.mRef);
        }
        for (const MWWorld::LiveCellRef<ESM4::Door>& door : cellStore.getReadOnlyEsm4Doors().mList)
        {
            if (!door.mRef.getTeleport())
                continue;
            sortedDoors.push_back(&door.mRef);
        }

        // Sort teleporting doors alphabetically, first by ID, then by destination cell to make search consistent
        std::sort(sortedDoors.begin(), sortedDoors.end(), [](const MWWorld::CellRef* lhs, const MWWorld::CellRef* rhs) {
            if (lhs->getRefId() != rhs->getRefId())
                return lhs->getRefId() < rhs->getRefId();

            return lhs->getDestCell() < rhs->getDestCell();
        });

        WorldModel* worldModel = MWBase::Environment::get().getWorldModel();
        for (const MWWorld::CellRef* door : sortedDoors)
        {
            const MWWorld::CellStore& source = worldModel->getCell(door->getDestCell());

            // Find door leading to our current teleport door
            // and use its destination to position inside cell.
            // \note Using _any_ door pointed to the cell,
            // not the one pointed to current door.
            for (const MWWorld::LiveCellRef<ESM::Door>& destDoor : source.getReadOnlyDoors().mList)
            {
                if (cellId == destDoor.mRef.getDestCell())
                {
                    ESM::Position doorDest = destDoor.mRef.getDoorDest();
                    doorDest.rot[0] = doorDest.rot[1] = doorDest.rot[2] = 0;
                    return doorDest;
                }
            }
            for (const MWWorld::LiveCellRef<ESM4::Door>& destDoor : source.getReadOnlyEsm4Doors().mList)
            {
                if (cellId == destDoor.mRef.getDestCell())
                    return destDoor.mRef.getDoorDest();
            }
        }

        return std::nullopt;
    }

    ESM::RefId World::findInteriorPosition(std::string_view name, ESM::Position& pos)
    {
        pos.rot[0] = pos.rot[1] = pos.rot[2] = 0;
        pos.pos[0] = pos.pos[1] = pos.pos[2] = 0;

        const MWWorld::CellStore* cellStore = mWorldModel.findInterior(name);
        if (!cellStore)
            return ESM::RefId();
        ESM::RefId cellId = cellStore->getCell()->getId();

        if (std::optional<ESM::Position> destPos = searchMarkerPosition(*cellStore, "cocmarkerheading"))
        {
            pos = *destPos;
            return cellId;
        }
        if (std::optional<ESM::Position> destPos = searchDoorDestInCell(*cellStore))
        {
            pos = *destPos;
            return cellId;
        }
        if (std::optional<ESM::Position> destPos = searchMarkerPosition(*cellStore, "xmarkerheading"))
        {
            pos = *destPos;
            return cellId;
        }

        // Fall back to the first static location.
        const MWWorld::CellRefList<ESM4::Static>::List& statics4 = cellStore->getReadOnlyEsm4Statics().mList;
        if (!statics4.empty())
        {
            pos = statics4.begin()->mRef.getPosition();
            pos.rot[0] = pos.rot[1] = pos.rot[2] = 0;
            return cellId;
        }
        const MWWorld::CellRefList<ESM::Static>::List& statics = cellStore->getReadOnlyStatics().mList;
        if (!statics.empty())
        {
            pos = statics.begin()->mRef.getPosition();
            pos.rot[0] = pos.rot[1] = pos.rot[2] = 0;
            return cellId;
        }

        return ESM::RefId();
    }

    ESM::RefId World::findExteriorPosition(std::string_view nameId, ESM::Position& pos)
    {
        pos.rot[0] = pos.rot[1] = pos.rot[2] = 0;

        const MWWorld::CellStore* cellStore = mWorldModel.findCell(nameId);

        if (cellStore != nullptr && !cellStore->isExterior())
            return ESM::RefId();

        if (!cellStore)
        {
            size_t comma = nameId.find(',');
            if (comma != std::string::npos)
            {
                int x, y;
                std::from_chars_result xResult = std::from_chars(nameId.data(), nameId.data() + comma, x);
                std::from_chars_result yResult
                    = std::from_chars(nameId.data() + comma + 1, nameId.data() + nameId.size(), y);
                if (xResult.ec == std::errc::result_out_of_range || yResult.ec == std::errc::result_out_of_range)
                    throw std::runtime_error("Cell coordinates out of range.");
                else if (xResult.ec == std::errc{} && yResult.ec == std::errc{})
                    cellStore
                        = &mWorldModel.getExterior(ESM::ExteriorCellLocation(x, y, ESM::Cell::sDefaultWorldspaceId));
                // ignore std::errc::invalid_argument, as this means that name probably refers to a interior cell
                // instead of comma separated coordinates
            }
        }

        if (!cellStore)
            return ESM::RefId();
        const MWWorld::Cell* ext = cellStore->getCell();

        if (std::optional<ESM::Position> destPos = searchMarkerPosition(*cellStore, "cocmarkerheading"))
        {
            pos = *destPos;
            return ext->getId();
        }
        if (std::optional<ESM::Position> destPos = searchMarkerPosition(*cellStore, "xmarkerheading"))
        {
            pos = *destPos;
            return ext->getId();
        }

        int x = ext->getGridX();
        int y = ext->getGridY();
        const osg::Vec2f posFromIndex = indexToPosition(ESM::ExteriorCellLocation(x, y, ext->getWorldSpace()), true);
        pos.pos[0] = posFromIndex.x();
        pos.pos[1] = posFromIndex.y();

        // Note: Z pos will be adjusted by adjustPosition later
        pos.pos[2] = 0;

        return ext->getId();
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

    bool World::getGodModeState() const
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

    void World::loadContentFiles(const Files::Collections& fileCollections, const std::vector<std::string>& content,
        ToUTF8::Utf8Encoder* encoder, Loading::Listener* listener)
    {
        GameContentLoader gameContentLoader;
        EsmLoader esmLoader(mStore, mReaders, encoder, mESMVersions);

        gameContentLoader.addLoader(".esm", esmLoader);
        gameContentLoader.addLoader(".esp", esmLoader);
        gameContentLoader.addLoader(".omwgame", esmLoader);
        gameContentLoader.addLoader(".omwaddon", esmLoader);
        gameContentLoader.addLoader(".project", esmLoader);

        OMWScriptsLoader omwScriptsLoader(mStore);
        gameContentLoader.addLoader(".omwscripts", omwScriptsLoader);

        int idx = 0;
        for (const std::string& file : content)
        {
            const auto filename = Files::pathFromUnicodeString(file);
            const Files::MultiDirCollection& col
                = fileCollections.getCollection(Files::pathToUnicodeString(filename.extension()));
            if (col.doesExist(file))
            {
                gameContentLoader.load(col.getPath(file), idx, listener);
            }
            else
            {
                std::string message = "Failed loading " + file + ": the content file does not exist";
                throw std::runtime_error(message);
            }
            idx++;
        }

        if (const auto v = esmLoader.getMasterFileFormat(); v.has_value() && *v == 0)
            ensureNeededRecords(); // Insert records that may not be present in all versions of master files.
    }

    void World::loadGroundcoverFiles(const Files::Collections& fileCollections,
        const std::vector<std::string>& groundcoverFiles, ToUTF8::Utf8Encoder* encoder, Loading::Listener* listener)
    {
        if (!Settings::groundcover().mEnabled)
            return;

        Log(Debug::Info) << "Loading groundcover:";

        mGroundcoverStore.init(mStore.get<ESM::Static>(), fileCollections, groundcoverFiles, encoder, listener);
    }

    MWWorld::SpellCastState World::startSpellCast(const Ptr& actor)
    {
        MWMechanics::CreatureStats& stats = actor.getClass().getCreatureStats(actor);

        std::string_view message;
        MWWorld::SpellCastState result = MWWorld::SpellCastState::Success;
        bool isPlayer = (actor == getPlayerPtr());

        const ESM::RefId& selectedSpell = stats.getSpells().getSelectedSpell();

        if (!selectedSpell.empty())
        {
            const ESM::Spell* spell = mStore.get<ESM::Spell>().find(selectedSpell);
            int spellCost = MWMechanics::calcSpellCost(*spell);

            // Check mana
            bool godmode = (isPlayer && mGodMode);
            MWMechanics::DynamicStat<float> magicka = stats.getMagicka();
            if (spellCost > 0 && magicka.getCurrent() < spellCost && !godmode)
            {
                message = "#{sMagicInsufficientSP}";
                result = MWWorld::SpellCastState::InsufficientMagicka;
            }

            // If this is a power, check if it was already used in the last 24h
            if (result == MWWorld::SpellCastState::Success && spell->mData.mType == ESM::Spell::ST_Power
                && !stats.getSpells().canUsePower(spell))
            {
                message = "#{sPowerAlreadyUsed}";
                result = MWWorld::SpellCastState::PowerAlreadyUsed;
            }

            if (result == MWWorld::SpellCastState::Success && !godmode)
            {
                // Reduce mana
                magicka.setCurrent(magicka.getCurrent() - spellCost);
                stats.setMagicka(magicka);

                // Reduce fatigue (note that in the vanilla game, both GMSTs are 0, and there's no fatigue loss)
                static const float fFatigueSpellBase
                    = mStore.get<ESM::GameSetting>().find("fFatigueSpellBase")->mValue.getFloat();
                static const float fFatigueSpellMult
                    = mStore.get<ESM::GameSetting>().find("fFatigueSpellMult")->mValue.getFloat();
                MWMechanics::DynamicStat<float> fatigue = stats.getFatigue();
                const float normalizedEncumbrance = actor.getClass().getNormalizedEncumbrance(actor);

                float fatigueLoss = spellCost * (fFatigueSpellBase + normalizedEncumbrance * fFatigueSpellMult);
                fatigue.setCurrent(fatigue.getCurrent() - fatigueLoss);
                stats.setFatigue(fatigue);
            }
        }

        if (isPlayer && result != MWWorld::SpellCastState::Success)
            MWBase::Environment::get().getWindowManager()->messageBox(message);

        return result;
    }

    void World::castSpell(const Ptr& actor, bool scriptedSpell)
    {
        MWMechanics::CreatureStats& stats = actor.getClass().getCreatureStats(actor);

        const bool casterIsPlayer = actor == MWMechanics::getPlayer();
        MWWorld::Ptr target;
        // For scripted spells we should not use hit contact
        if (scriptedSpell)
        {
            if (!casterIsPlayer)
            {
                for (const auto& package : stats.getAiSequence())
                {
                    if (package->getTypeId() == MWMechanics::AiPackageTypeId::Cast)
                    {
                        target = package->getTarget();
                        break;
                    }
                }
            }
        }
        else
        {
            if (casterIsPlayer)
                target = getFacedObject();

            if (target.isEmpty() || !target.getClass().hasToolTip(target))
            {
                // For actor targets, we want to use melee hit contact.
                // This is to give a slight tolerance for errors, especially with creatures like the Skeleton that would
                // be very hard to aim at otherwise.
                // For object targets, we want the detailed shapes (rendering raycast).
                // If we used the bounding boxes for static objects, then we would not be able to target e.g.
                // objects lying on a shelf.
                const float fCombatDistance = mStore.get<ESM::GameSetting>().find("fCombatDistance")->mValue.getFloat();
                target = MWMechanics::getHitContact(actor, fCombatDistance).first;

                if (target.isEmpty())
                {
                    // Get the target using the facing direction from Head node
                    const osg::Vec3f origin = getActorHeadTransform(actor).getTrans();
                    const osg::Quat orient = osg::Quat(actor.getRefData().getPosition().rot[0], osg::Vec3f(-1, 0, 0))
                        * osg::Quat(actor.getRefData().getPosition().rot[2], osg::Vec3f(0, 0, -1));
                    const osg::Vec3f direction = orient * osg::Vec3f(0, 1, 0);
                    const osg::Vec3f dest = origin + direction * getMaxActivationDistance();
                    const MWRender::RenderingManager::RayResult result = mRendering->castRay(origin, dest, true, true);
                    if (result.mHit)
                        target = result.mHitObject;
                }
            }
        }

        osg::Vec3f hitPosition = actor.getRefData().getPosition().asVec3();
        if (!target.isEmpty())
        {
            // Touch explosion placement doesn't depend on where the target was "touched".
            // In Morrowind, it's at 0.7 of the actor's AABB height for actors
            // or at 0.7 of the player's height for non-actors if the player is the caster
            // This is probably meant to prevent the explosion from being too far above on large objects
            // but it often puts the explosions way above small objects, so we'll deviate here
            // and use the object's bounds when reasonable (it's $CURRENT_YEAR, we can afford that)
            // Note collision object origin is intentionally not used
            hitPosition = target.getRefData().getPosition().asVec3();
            constexpr float explosionHeight = 0.7f;
            float targetHeight = getHalfExtents(target).z() * 2.f;
            if (!target.getClass().isActor() && casterIsPlayer)
            {
                const float playerHeight = getHalfExtents(actor).z() * 2.f;
                targetHeight = std::min(targetHeight, playerHeight);
            }
            hitPosition.z() += targetHeight * explosionHeight;
        }

        const ESM::RefId& selectedSpell = stats.getSpells().getSelectedSpell();

        MWMechanics::CastSpell cast(actor, target, false, scriptedSpell);
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
            {
                const auto& itemPtr = *inv.getSelectedEnchantItem();
                cast.cast(itemPtr);
            }
        }
    }

    void World::launchProjectile(MWWorld::Ptr& actor, MWWorld::Ptr& projectile, const osg::Vec3f& worldPos,
        const osg::Quat& orient, MWWorld::Ptr& bow, float speed, float attackStrength)
    {
        // An initial position of projectile can be outside shooter's collision box, so any object between shooter and
        // launch position will be ignored. To avoid this issue, we should check for impact immediately before launch
        // the projectile. So we cast a 1-yard-length ray from shooter to launch position and check if there are
        // collisions in this area.
        // TODO: as a better solutuon we should handle projectiles during physics update, not during world update.
        const osg::Vec3f sourcePos = worldPos + orient * osg::Vec3f(0, -1, 0) * 64.f;

        // Early out if the launch position is underwater
        bool underwater = isUnderwater(MWMechanics::getPlayer().getCell(), worldPos);
        if (underwater)
        {
            MWMechanics::projectileHit(actor, Ptr(), bow, projectile, worldPos, attackStrength);
            mRendering->emitWaterRipple(worldPos);
            return;
        }

        // For AI actors, get combat targets to use in the ray cast. Only those targets will return a positive hit
        // result.
        std::vector<MWWorld::Ptr> targetActors;
        if (!actor.isEmpty() && actor.getClass().isActor() && actor != MWMechanics::getPlayer())
            actor.getClass().getCreatureStats(actor).getAiSequence().getCombatTargets(targetActors);

        // Check for impact, if yes, handle hit, if not, launch projectile
        MWPhysics::RayCastingResult result = mPhysics->castRay(
            sourcePos, worldPos, { actor }, targetActors, 0xff, MWPhysics::CollisionType_Projectile);
        if (result.mHit)
            MWMechanics::projectileHit(actor, result.mHitObject, bow, projectile, result.mHitPos, attackStrength);
        else
            mProjectileManager->launchProjectile(actor, projectile, worldPos, orient, bow, speed, attackStrength);
    }

    void World::launchMagicBolt(
        const ESM::RefId& spellId, const MWWorld::Ptr& caster, const osg::Vec3f& fallbackDirection, ESM::RefNum item)
    {
        mProjectileManager->launchMagicBolt(spellId, caster, fallbackDirection, item);
    }

    void World::updateProjectilesCasters()
    {
        mProjectileManager->updateCasters();
    }

    void World::applyLoopingParticles(const MWWorld::Ptr& ptr) const
    {
        const MWWorld::Class& cls = ptr.getClass();
        if (cls.isActor())
        {
            std::set<int> playing;
            for (const auto& params : cls.getCreatureStats(ptr).getActiveSpells())
            {
                for (const auto& effect : params.getEffects())
                {
                    if (playing.insert(effect.mEffectId).second)
                    {
                        const auto magicEffect = mStore.get<ESM::MagicEffect>().find(effect.mEffectId);
                        if (magicEffect->mData.mFlags & ESM::MagicEffect::ContinuousVfx)
                            MWMechanics::playEffects(ptr, *magicEffect, false);
                    }
                }
            }
        }
    }

    const std::vector<std::string>& World::getContentFiles() const
    {
        return mContentFiles;
    }

    void World::breakInvisibility(const Ptr& actor)
    {
        actor.getClass().getCreatureStats(actor).getActiveSpells().purgeEffect(actor, ESM::MagicEffect::Invisibility);

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
            const MWWorld::Cell& cellVariant = *cell->getCell();
            uint32_t ambient = cellVariant.getMood().mAmbiantColor;
            int ambientTotal = (ambient & 0xff) + ((ambient >> 8) & 0xff) + ((ambient >> 16) & 0xff);
            return !cell->getCell()->noSleep() && ambientTotal <= 201;
        }
    }

    float World::getSunVisibility() const
    {
        return mWeatherManager->getSunVisibility();
    }

    float World::getSunPercentage() const
    {
        return mWeatherManager->getSunPercentage(getTimeStamp().getHour());
    }

    bool World::findInteriorPositionInWorldSpace(const MWWorld::CellStore* cell, osg::Vec3f& result)
    {
        if (cell->isExterior())
            return false;

        // Search for a 'nearest' exterior, counting each cell between the starting
        // cell and the exterior as a distance of 1.  Will fail for isolated interiors.
        std::set<ESM::RefId> checkedCells;
        std::set<ESM::RefId> currentCells;
        std::set<ESM::RefId> nextCells;
        nextCells.insert(cell->getCell()->getId());

        while (!nextCells.empty())
        {
            currentCells = nextCells;
            nextCells.clear();
            for (const auto& currentCell : currentCells)
            {
                MWWorld::CellStore& next = mWorldModel.getCell(currentCell);

                // Check if any door in the cell leads to an exterior directly
                for (const MWWorld::LiveCellRef<ESM::Door>& ref : next.getReadOnlyDoors().mList)
                {
                    if (!ref.mRef.getTeleport())
                        continue;

                    if (ref.mRef.getDestCell().is<ESM::ESM3ExteriorCellRefId>())
                    {
                        ESM::Position pos = ref.mRef.getDoorDest();
                        result = pos.asVec3();
                        return true;
                    }
                    else
                    {
                        ESM::RefId dest = ref.mRef.getDestCell();
                        if (!checkedCells.count(dest) && !currentCells.count(dest))
                            nextCells.insert(dest);
                    }
                }

                checkedCells.insert(currentCell);
            }
        }

        // No luck :(
        return false;
    }

    MWWorld::ConstPtr World::getClosestMarker(const MWWorld::ConstPtr& ptr, const ESM::RefId& id)
    {
        if (ptr.getCell()->isExterior())
        {
            return getClosestMarkerFromExteriorPosition(mPlayer->getLastKnownExteriorPosition(), id);
        }

        // Search for a 'nearest' marker, counting each cell between the starting
        // cell and the exterior as a distance of 1.  If an exterior is found, jump
        // to the nearest exterior marker, without further interior searching.
        std::set<ESM::RefId> checkedCells;
        std::set<ESM::RefId> currentCells;
        std::set<ESM::RefId> nextCells;
        MWWorld::ConstPtr closestMarker;

        nextCells.insert(ptr.getCell()->getCell()->getId());
        while (!nextCells.empty())
        {
            currentCells.clear();
            std::swap(currentCells, nextCells);
            for (const auto& cell : currentCells)
            {
                MWWorld::CellStore& next = mWorldModel.getCell(cell);
                checkedCells.insert(cell);

                closestMarker = next.searchConst(id);
                if (!closestMarker.isEmpty())
                {
                    return closestMarker;
                }

                // Check if any door in the cell leads to an exterior directly
                for (const MWWorld::LiveCellRef<ESM::Door>& ref : next.getReadOnlyDoors().mList)
                {
                    if (!ref.mRef.getTeleport())
                        continue;

                    if (ref.mRef.getDestCell().is<ESM::ESM3ExteriorCellRefId>())
                    {
                        osg::Vec3f worldPos = ref.mRef.getDoorDest().asVec3();
                        return getClosestMarkerFromExteriorPosition(worldPos, id);
                    }
                    else
                    {
                        const auto& dest = ref.mRef.getDestCell();
                        if (!checkedCells.contains(dest) && !currentCells.contains(dest))
                            nextCells.insert(dest);
                    }
                }
            }
        }
        return MWWorld::Ptr();
    }

    MWWorld::ConstPtr World::getClosestMarkerFromExteriorPosition(const osg::Vec3f& worldPos, const ESM::RefId& id)
    {
        const ESM::ExteriorCellLocation posIndex = ESM::positionToExteriorCellLocation(worldPos.x(), worldPos.y());

        // Potential optimization: don't scan the entire world for markers and actually do the Todd spiral
        std::vector<Ptr> markers;
        mWorldModel.getExteriorPtrs(id, markers);

        struct MarkerInfo
        {
            Ptr mPtr;
            int mColumn, mRow; // Local coordinates in the valid marker grid
        };
        std::vector<MarkerInfo> validMarkers;
        validMarkers.reserve(markers.size());

        // The idea is to collect all markers that belong to the smallest possible square grid around worldPos
        // They are grouped with their position on that grid's edge where the origin is the SW corner
        int minGridSize = std::numeric_limits<int>::max();
        for (const Ptr& marker : markers)
        {
            const osg::Vec3f markerPos = marker.getRefData().getPosition().asVec3();
            const ESM::ExteriorCellLocation index = ESM::positionToExteriorCellLocation(markerPos.x(), markerPos.y());

            const int deltaX = index.mX - posIndex.mX;
            const int deltaY = index.mY - posIndex.mY;
            const int gridSize = std::max(std::abs(deltaX), std::abs(deltaY)) * 2;
            if (gridSize == 0)
                return marker;

            if (gridSize <= minGridSize)
            {
                if (gridSize < minGridSize)
                {
                    validMarkers.clear();
                    minGridSize = gridSize;
                }
                validMarkers.push_back({ marker, gridSize / 2 + deltaX, gridSize / 2 + deltaY });
            }
        }

        ConstPtr closestMarker;
        if (validMarkers.empty())
            return closestMarker;
        if (validMarkers.size() == 1)
            return validMarkers[0].mPtr;

        // All the markers are on the edge of the grid
        // Break ties by picking the earliest marker on SW -> SE -> NE -> NW -> SW path
        int earliestDistance = std::numeric_limits<int>::max();
        for (const MarkerInfo& marker : validMarkers)
        {
            int distance = 0;
            if (marker.mRow == 0) // South edge (plus SW and SE corners)
                distance = marker.mColumn;
            else if (marker.mColumn == minGridSize) // East edge and NE corner
                distance = minGridSize + marker.mRow;
            else if (marker.mRow == minGridSize) // North edge and NW corner
                distance = minGridSize * 3 - marker.mColumn;
            else // West edge
                distance = minGridSize * 4 - marker.mRow;
            if (distance < earliestDistance)
            {
                closestMarker = marker.mPtr;
                earliestDistance = distance;
            }
        }
        return closestMarker;
    }

    void World::rest(double hours)
    {
        mWorldModel.forEachLoadedCellStore([hours](CellStore& store) { store.rest(hours); });
    }

    void World::rechargeItems(double duration, bool activeOnly)
    {
        MWWorld::Ptr player = getPlayerPtr();
        player.getClass().getInventoryStore(player).rechargeItems(duration);

        if (activeOnly)
        {
            for (auto& cell : mWorldScene->getActiveCells())
            {
                cell->recharge(duration);
            }
        }
        else
            mWorldModel.forEachLoadedCellStore([duration](CellStore& store) { store.recharge(duration); });
    }

    void World::teleportToClosestMarker(const MWWorld::Ptr& ptr, const ESM::RefId& id)
    {
        MWWorld::ConstPtr closestMarker = getClosestMarker(ptr, id);

        if (closestMarker.isEmpty())
        {
            Log(Debug::Warning) << "Failed to teleport: no closest marker found";
            return;
        }

        ESM::RefId cellId = closestMarker.mCell->getCell()->getId();

        MWWorld::ActionTeleport action(cellId, closestMarker.getRefData().getPosition(), false);
        action.execute(ptr);
    }

    void World::updateWeather(float duration, bool paused)
    {
        bool isExterior = isCellExterior() || isCellQuasiExterior();
        if (mPlayer->wasTeleported())
        {
            mPlayer->setTeleported(false);

            const ESM::RefId& playerRegion = getPlayerPtr().getCell()->getCell()->getRegion();
            mWeatherManager->playerTeleported(playerRegion, isExterior);
        }

        const TimeStamp time = getTimeStamp();
        mWeatherManager->update(duration, paused, time, isExterior);
    }

    struct AddDetectedReferenceVisitor
    {
        std::vector<Ptr>& mOut;
        Ptr mDetector;
        float mSquaredDist;
        World::DetectionType mType;
        const MWWorld::ESMStore& mStore;

        bool operator()(const MWWorld::Ptr& ptr)
        {
            if ((ptr.getRefData().getPosition().asVec3() - mDetector.getRefData().getPosition().asVec3()).length2()
                >= mSquaredDist)
                return true;

            if (!ptr.getRefData().isEnabled() || ptr.mRef->isDeleted())
                return true;

            // Consider references inside containers as well (except if we are looking for a Creature, they cannot be in
            // containers)
            bool isContainer = ptr.getClass().getType() == ESM::Container::sRecordId;
            if (mType != World::Detect_Creature && (ptr.getClass().isActor() || isContainer))
            {
                // but ignore containers without resolved content
                if (isContainer && ptr.getRefData().getCustomData() == nullptr)
                {
                    for (const auto& containerItem : ptr.get<ESM::Container>()->mBase->mInventory.mList)
                    {
                        if (containerItem.mCount)
                        {
                            try
                            {
                                ManualRef ref(mStore, containerItem.mItem, containerItem.mCount);
                                if (needToAdd(ref.getPtr(), mDetector))
                                {
                                    mOut.push_back(ptr);
                                    return true;
                                }
                            }
                            catch (const std::exception& e)
                            {
                                Log(Debug::Warning)
                                    << "Failed to process container item " << containerItem.mItem << ": " << e.what();
                            }
                        }
                    }
                    return true;
                }

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

        bool needToAdd(const MWWorld::Ptr& ptr, const MWWorld::Ptr& detector)
        {
            if (mType == World::Detect_Creature)
            {
                // If in werewolf form, this detects only NPCs, otherwise only creatures
                if (detector.getClass().isNpc() && detector.getClass().getNpcStats(detector).isWerewolf())
                {
                    if (ptr.getClass().getType() != ESM::NPC::sRecordId)
                        return false;
                }
                else if (ptr.getClass().getType() != ESM::Creature::sRecordId)
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

    void World::listDetectedReferences(const Ptr& ptr, std::vector<Ptr>& out, DetectionType type)
    {
        const MWMechanics::MagicEffects& effects = ptr.getClass().getCreatureStats(ptr).getMagicEffects();
        float dist = 0;
        if (type == World::Detect_Creature)
            dist = effects.getOrDefault(ESM::MagicEffect::DetectAnimal).getMagnitude();
        else if (type == World::Detect_Key)
            dist = effects.getOrDefault(ESM::MagicEffect::DetectKey).getMagnitude();
        else if (type == World::Detect_Enchantment)
            dist = effects.getOrDefault(ESM::MagicEffect::DetectEnchantment).getMagnitude();

        if (!dist)
            return;

        dist = feetToGameUnits(dist);

        AddDetectedReferenceVisitor visitor{ out, ptr, dist * dist, type, mStore };

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
        float telekinesisRangeBonus = mPlayer->getPlayer()
                                          .getClass()
                                          .getCreatureStats(mPlayer->getPlayer())
                                          .getMagicEffects()
                                          .getOrDefault(ESM::MagicEffect::Telekinesis)
                                          .getMagnitude();
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

        static float fCrimeGoldDiscountMult
            = mStore.get<ESM::GameSetting>().find("fCrimeGoldDiscountMult")->mValue.getFloat();
        static float fCrimeGoldTurnInMult
            = mStore.get<ESM::GameSetting>().find("fCrimeGoldTurnInMult")->mValue.getFloat();

        int discount = static_cast<int>(bounty * fCrimeGoldDiscountMult);
        int turnIn = static_cast<int>(bounty * fCrimeGoldTurnInMult);

        if (bounty > 0)
        {
            discount = std::max(1, discount);
            turnIn = std::max(1, turnIn);
        }

        mGlobalVariables[Globals::sPCHasCrimeGold].setInteger((bounty <= playerGold) ? 1 : 0);

        mGlobalVariables[Globals::sPCHasGoldDiscount].setInteger((discount <= playerGold) ? 1 : 0);
        mGlobalVariables[Globals::sCrimeGoldDiscount].setInteger(discount);

        mGlobalVariables[Globals::sCrimeGoldTurnIn].setInteger(turnIn);
        mGlobalVariables[Globals::sPCHasTurnIn].setInteger((turnIn <= playerGold) ? 1 : 0);
    }

    void World::confiscateStolenItems(const Ptr& ptr)
    {
        MWWorld::ConstPtr prisonMarker = getClosestMarker(ptr, ESM::RefId::stringRefId("prisonmarker"));
        if (prisonMarker.isEmpty())
        {
            Log(Debug::Warning) << "Failed to confiscate items: no closest prison marker found.";
            return;
        }
        ESM::RefId prisonName = prisonMarker.getCellRef().getDestCell();
        if (prisonName.empty())
        {
            Log(Debug::Warning) << "Failed to confiscate items: prison marker not linked to prison interior";
            return;
        }
        MWWorld::CellStore& prison = mWorldModel.getCell(prisonName);

        MWWorld::Ptr closestChest = prison.search(ESM::RefId::stringRefId("stolen_goods"));
        if (!closestChest.isEmpty()) // Found a close chest
        {
            MWBase::Environment::get().getMechanicsManager()->confiscateStolenItems(ptr, closestChest);
        }
        else
            Log(Debug::Warning) << "Failed to confiscate items: no stolen_goods container found";
    }

    void World::goToJail()
    {
        const MWWorld::Ptr player = getPlayerPtr();
        if (!mGoToJail)
        {
            // Reset bounty and forget the crime now, but don't change cell yet (the player should be able to read the
            // dialog text first)
            mGoToJail = true;
            mPlayerInJail = true;

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
            if (MWBase::Environment::get().getMechanicsManager()->isAttackPreparing(player))
            {
                player.getClass().getCreatureStats(player).setAttackingOrSpell(false);
            }

            mPlayer->setDrawState(MWMechanics::DrawState::Nothing);
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

    float World::getTerrainHeightAt(const osg::Vec3f& worldPos, ESM::RefId worldspace) const
    {
        return mRendering->getTerrainHeightAt(worldPos, worldspace);
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

    std::filesystem::path World::exportSceneGraph(const Ptr& ptr)
    {
        auto file = mUserDataPath / "openmw.osgt";
        if (!ptr.isEmpty())
        {
            mRendering->pagingBlacklistObject(mStore.find(ptr.getCellRef().getRefId()), ptr);
            mWorldScene->removeFromPagedRefs(ptr);
        }
        mRendering->exportSceneGraph(ptr, file, "Ascii");
        return file;
    }

    void World::spawnRandomCreature(const ESM::RefId& creatureList)
    {
        const ESM::CreatureLevList* list = mStore.get<ESM::CreatureLevList>().find(creatureList);

        static int iNumberCreatures = mStore.get<ESM::GameSetting>().find("iNumberCreatures")->mValue.getInteger();
        int numCreatures = 1 + Misc::Rng::rollDice(iNumberCreatures, mPrng); // [1, iNumberCreatures]

        for (int i = 0; i < numCreatures; ++i)
        {
            const ESM::RefId& selectedCreature = MWMechanics::getLevelledItem(list, true, mPrng);
            if (selectedCreature.empty())
                continue;

            MWWorld::ManualRef ref(mStore, selectedCreature, 1);

            safePlaceObject(ref.getPtr(), getPlayerPtr(), getPlayerPtr().getCell(), 0, 220.f);
        }
    }

    void World::spawnBloodEffect(const Ptr& ptr, const osg::Vec3f& worldPosition)
    {
        if (ptr == getPlayerPtr() && Settings::gui().mHitFader)
            return;

        std::string_view texture
            = Fallback::Map::getString("Blood_Texture_" + std::to_string(ptr.getClass().getBloodTexture(ptr)));
        if (texture.empty())
            texture = Fallback::Map::getString("Blood_Texture_0");

        VFS::Path::Normalized model(Misc::ResourceHelpers::correctMeshPath(std::string{
            Fallback::Map::getString("Blood_Model_" + std::to_string(Misc::Rng::rollDice(3))) } /*[0, 2]*/));

        mRendering->spawnEffect(model, texture, worldPosition, 1.0f, false);
    }

    void World::spawnEffect(VFS::Path::NormalizedView model, const std::string& textureOverride,
        const osg::Vec3f& worldPos, float scale, bool isMagicVFX)
    {
        mRendering->spawnEffect(model, textureOverride, worldPos, scale, isMagicVFX);
    }

    struct ResetActorsVisitor
    {
        World& mWorld;

        bool operator()(const Ptr& ptr)
        {
            if (ptr.getClass().isActor() && ptr.getCellRef().hasContentFile())
            {
                if (ptr.getCell()->movedHere(ptr))
                    return true;

                const ESM::Position& origPos = ptr.getCellRef().getPosition();
                mWorld.moveObject(ptr, origPos.asVec3());
                mWorld.rotateObject(ptr, origPos.asRotationVec3());
                ptr.getClass().adjustPosition(ptr, true);
            }
            return true;
        }
    };

    void World::resetActors()
    {
        for (CellStore* cellstore : mWorldScene->getActiveCells())
        {
            ResetActorsVisitor visitor{ *this };
            cellstore->forEach(visitor);
        }
    }

    bool World::isWalkingOnWater(const ConstPtr& actor) const
    {
        const MWPhysics::Actor* physicActor = mPhysics->getActor(actor);
        if (physicActor && physicActor->isWalkingOnWater())
            return true;
        return false;
    }

    osg::Vec3f World::aimToTarget(const ConstPtr& actor, const ConstPtr& target, bool isRangedCombat)
    {
        osg::Vec3f weaponPos = actor.getRefData().getPosition().asVec3();
        float heightRatio = isRangedCombat ? 2.f * Constants::TorsoHeight : 1.f;
        weaponPos.z() += mPhysics->getHalfExtents(actor).z() * heightRatio;
        osg::Vec3f targetPos = mPhysics->getCollisionObjectPosition(target);
        return (targetPos - weaponPos);
    }

    namespace
    {
        void preload(MWWorld::Scene* scene, const ESMStore& store, const ESM::RefId& obj)
        {
            if (obj.empty())
                return;
            try
            {
                MWWorld::ManualRef ref(store, obj);
                std::string model = ref.getPtr().getClass().getCorrectedModel(ref.getPtr());
                if (!model.empty())
                    scene->preload(model, ref.getPtr().getClass().useAnim());
            }
            catch (const std::exception& e)
            {
                Log(Debug::Warning) << "Failed to preload scene object " << obj << ": " << e.what();
            }
        }
    }

    void World::preloadEffects(const ESM::EffectList* effectList)
    {
        for (const ESM::IndexedENAMstruct& effectInfo : effectList->mList)
        {
            const ESM::MagicEffect* effect = mStore.get<ESM::MagicEffect>().find(effectInfo.mData.mEffectID);

            if (MWMechanics::isSummoningEffect(effectInfo.mData.mEffectID))
            {
                preload(mWorldScene.get(), mStore, ESM::RefId::stringRefId("VFX_Summon_Start"));
                preload(mWorldScene.get(), mStore, MWMechanics::getSummonedCreature(effectInfo.mData.mEffectID));
            }

            preload(mWorldScene.get(), mStore, effect->mCasting);
            preload(mWorldScene.get(), mStore, effect->mHit);

            if (effectInfo.mData.mArea > 0)
                preload(mWorldScene.get(), mStore, effect->mArea);
            if (effectInfo.mData.mRange == ESM::RT_Target)
                preload(mWorldScene.get(), mStore, effect->mBolt);
        }
    }

    DetourNavigator::Navigator* World::getNavigator() const
    {
        return mNavigator.get();
    }

    void World::updateActorPath(const MWWorld::ConstPtr& actor, const std::deque<osg::Vec3f>& path,
        const DetourNavigator::AgentBounds& agentBounds, const osg::Vec3f& start, const osg::Vec3f& end) const
    {
        mRendering->updateActorPath(actor, path, agentBounds, start, end);
    }

    void World::removeActorPath(const MWWorld::ConstPtr& actor) const
    {
        mRendering->removeActorPath(actor);
    }

    void World::setNavMeshNumberToRender(const std::size_t value)
    {
        mRendering->setNavMeshNumber(value);
    }

    DetourNavigator::AgentBounds World::getPathfindingAgentBounds(const MWWorld::ConstPtr& actor) const
    {
        const MWPhysics::Actor* physicsActor = mPhysics->getActor(actor);
        if (physicsActor == nullptr || !actor.isInCell() || actor.getCell()->isExterior())
            return DetourNavigator::AgentBounds{ Settings::game().mActorCollisionShapeType,
                Settings::game().mDefaultActorPathfindHalfExtents };
        else
            return DetourNavigator::AgentBounds{ physicsActor->getCollisionShapeType(),
                physicsActor->getHalfExtents() };
    }

    bool World::hasCollisionWithDoor(
        const MWWorld::ConstPtr& door, const osg::Vec3f& position, const osg::Vec3f& destination) const
    {
        const auto object = mPhysics->getObject(door);

        if (!object)
            return false;

        btVector3 aabbMin;
        btVector3 aabbMax;
        object->getShapeInstance()->mCollisionShape->getAabb(btTransform::getIdentity(), aabbMin, aabbMax);

        const auto toLocal = object->getTransform().inverse();
        const auto localFrom = toLocal(Misc::Convert::toBullet(position));
        const auto localTo = toLocal(Misc::Convert::toBullet(destination));

        btScalar hitDistance = 1;
        btVector3 hitNormal;
        return btRayAabb(localFrom, localTo, aabbMin, aabbMax, hitDistance, hitNormal);
    }

    bool World::isAreaOccupiedByOtherActor(const osg::Vec3f& position, const float radius,
        std::span<const MWWorld::ConstPtr> ignore, std::vector<MWWorld::Ptr>* occupyingActors) const
    {
        return mPhysics->isAreaOccupiedByOtherActor(position, radius, ignore, occupyingActors);
    }

    void World::reportStats(unsigned int frameNumber, osg::Stats& stats) const
    {
        DetourNavigator::reportStats(mNavigator->getStats(), frameNumber, stats);
        mPhysics->reportStats(frameNumber, stats);
        mWorldScene->reportStats(frameNumber, stats);
    }

    std::vector<MWWorld::Ptr> World::getAll(const ESM::RefId& id)
    {
        return mWorldModel.getAll(id);
    }

    Misc::Rng::Generator& World::getPrng()
    {
        return mPrng;
    }

    MWRender::PostProcessor* World::getPostProcessor()
    {
        return mRendering->getPostProcessor();
    }

    void World::setActorActive(const MWWorld::Ptr& ptr, bool value)
    {
        if (MWPhysics::Actor* const actor = mPhysics->getActor(ptr))
            actor->setActive(value);
    }
}
