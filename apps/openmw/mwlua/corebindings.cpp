#include "corebindings.hpp"

#include <chrono>
#include <stdexcept>
#include <tuple>

#include <sol/object.hpp>

#include <apps/openmw/mwlua/object.hpp>
#include <apps/openmw/mwworld/cellstore.hpp>
#include <apps/openmw/mwworld/worldmodel.hpp>

#include <components/debug/debuglog.hpp>
#include <components/esm3/landrecorddata.hpp>
#include <components/esm3/loadfact.hpp>
#include <components/esm3/loadland.hpp>
#include <components/esm3/loadltex.hpp>
#include <components/esmterrain/storage.hpp>
#include <components/lua/l10n.hpp>
#include <components/lua/luastate.hpp>
#include <components/lua/serialization.hpp>
#include <components/lua/util.hpp>
#include <components/misc/strings/algorithm.hpp>
#include <components/misc/strings/lower.hpp>
#include <components/version/version.hpp>

#include "../mwbase/environment.hpp"
#include "../mwbase/statemanager.hpp"
#include "../mwbase/world.hpp"
#include "../mwworld/datetimemanager.hpp"
#include "../mwworld/esmstore.hpp"

#include "dialoguebindings.hpp"
#include "factionbindings.hpp"
#include "luaevents.hpp"
#include "magicbindings.hpp"
#include "soundbindings.hpp"
#include "stats.hpp"

namespace MWLua
{
    static sol::table initContentFilesBindings(sol::state_view& lua)
    {
        const std::vector<std::string>& contentList = MWBase::Environment::get().getWorld()->getContentFiles();
        sol::table list(lua, sol::create);
        for (size_t i = 0; i < contentList.size(); ++i)
            list[LuaUtil::toLuaIndex(i)] = Misc::StringUtils::lowerCase(contentList[i]);
        sol::table res(lua, sol::create);
        res["list"] = LuaUtil::makeReadOnly(list);
        res["indexOf"] = [&contentList](std::string_view contentFile) -> sol::optional<int> {
            for (size_t i = 0; i < contentList.size(); ++i)
                if (Misc::StringUtils::ciEqual(contentList[i], contentFile))
                    return LuaUtil::toLuaIndex(i);
            return sol::nullopt;
        };
        res["has"] = [&contentList](std::string_view contentFile) -> bool {
            for (size_t i = 0; i < contentList.size(); ++i)
                if (Misc::StringUtils::ciEqual(contentList[i], contentFile))
                    return true;
            return false;
        };
        return LuaUtil::makeReadOnly(res);
    }

    void addCoreTimeBindings(sol::table& api, const Context& context)
    {
        MWWorld::DateTimeManager* timeManager = MWBase::Environment::get().getWorld()->getTimeManager();

        api["getSimulationTime"] = [timeManager]() { return timeManager->getSimulationTime(); };
        api["getSimulationTimeScale"] = [timeManager]() { return timeManager->getSimulationTimeScale(); };
        api["getGameTime"] = [timeManager]() { return timeManager->getGameTime(); };
        api["getGameTimeScale"] = [timeManager]() { return timeManager->getGameTimeScale(); };
        api["isWorldPaused"] = [timeManager]() { return timeManager->isPaused(); };
        api["getRealTime"] = []() {
            return std::chrono::duration<double>(std::chrono::steady_clock::now().time_since_epoch()).count();
        };

        if (context.mType != Context::Global)
            api["getRealFrameDuration"] = []() { return MWBase::Environment::get().getFrameDuration(); };
    }

    sol::table initCorePackage(const Context& context)
    {
        auto lua = context.sol();
        sol::object cached = context.getTypePackage("openmw_core");
        if (cached != sol::nil)
            return cached;

        sol::table api(lua, sol::create);
        api["API_REVISION"] = Version::getLuaApiRevision(); // specified in CMakeLists.txt
        api["quit"] = [lua = context.mLua]() {
            Log(Debug::Warning) << "Quit requested by a Lua script.\n" << lua->debugTraceback();
            MWBase::Environment::get().getStateManager()->requestQuit();
        };
        api["contentFiles"] = initContentFilesBindings(lua);
        api["getFormId"] = [](std::string_view contentFile, unsigned int index) -> std::string {
            const std::vector<std::string>& contentList = MWBase::Environment::get().getWorld()->getContentFiles();
            for (size_t i = 0; i < contentList.size(); ++i)
                if (Misc::StringUtils::ciEqual(contentList[i], contentFile))
                    return ESM::RefId(ESM::FormId{ index, int(i) }).serializeText();
            throw std::runtime_error("Content file not found: " + std::string(contentFile));
        };
        addCoreTimeBindings(api, context);

        api["magic"]
            = context.cachePackage("openmw_core_magic", [context]() { return initCoreMagicBindings(context); });

        api["stats"]
            = context.cachePackage("openmw_core_stats", [context]() { return initCoreStatsBindings(context); });

        api["factions"]
            = context.cachePackage("openmw_core_factions", [context]() { return initCoreFactionBindings(context); });
        api["dialogue"]
            = context.cachePackage("openmw_core_dialogue", [context]() { return initCoreDialogueBindings(context); });
        api["l10n"] = context.cachePackage("openmw_core_l10n",
            [lua]() { return LuaUtil::initL10nLoader(lua, MWBase::Environment::get().getL10nManager()); });
        const MWWorld::Store<ESM::GameSetting>* gmstStore
            = &MWBase::Environment::get().getESMStore()->get<ESM::GameSetting>();
        api["getGMST"] = [lua, gmstStore](const std::string& setting) -> sol::object {
            const ESM::GameSetting* gmst = gmstStore->search(setting);
            if (gmst == nullptr)
                return sol::nil;
            const ESM::Variant& value = gmst->mValue;
            switch (value.getType())
            {
                case ESM::VT_Float:
                    return sol::make_object<float>(lua, value.getFloat());
                case ESM::VT_Short:
                case ESM::VT_Long:
                case ESM::VT_Int:
                    return sol::make_object<int>(lua, value.getInteger());
                case ESM::VT_String:
                    return sol::make_object<std::string>(lua, value.getString());
                case ESM::VT_Unknown:
                case ESM::VT_None:
                    break;
            }
            return sol::nil;
        };

        if (context.mType != Context::Menu)
        {
            api["sendGlobalEvent"] = [context](std::string eventName, const sol::object& eventData) {
                context.mLuaEvents->addGlobalEvent(
                    { std::move(eventName), LuaUtil::serialize(eventData, context.mSerializer) });
            };
            api["sound"]
                = context.cachePackage("openmw_core_sound", [context]() { return initCoreSoundBindings(context); });
        }
        else
        {
            api["sendGlobalEvent"] = [context](std::string eventName, const sol::object& eventData) {
                if (MWBase::Environment::get().getStateManager()->getState() == MWBase::StateManager::State_NoGame)
                {
                    throw std::logic_error("Can't send global events when no game is loaded");
                }
                context.mLuaEvents->addGlobalEvent(
                    { std::move(eventName), LuaUtil::serialize(eventData, context.mSerializer) });
            };
        }

        api["getHeightAt"] = [](const osg::Vec3f& pos, sol::object cellOrName) {
            ESM::RefId worldspace;
            if (cellOrName.is<GCell>())
                worldspace = cellOrName.as<GCell>().mStore->getCell()->getWorldSpace();
            else if (cellOrName.is<std::string_view>() && !cellOrName.as<std::string_view>().empty())
                worldspace = MWBase::Environment::get()
                                 .getWorldModel()
                                 ->getCell(cellOrName.as<std::string_view>())
                                 .getCell()
                                 ->getWorldSpace();
            else
                worldspace = ESM::Cell::sDefaultWorldspaceId;

            const float cellSize = ESM::getCellSize(worldspace);
            int cellX = static_cast<int>(std::floor(pos.x() / cellSize));
            int cellY = static_cast<int>(std::floor(pos.y() / cellSize));

            auto store = MWBase::Environment::get().getESMStore();
            auto landStore = store->get<ESM::Land>();
            auto land = landStore.search(cellX, cellY);
            const ESM::Land::LandData* landData = nullptr;
            if (land != nullptr)
            {
                landData = land->getLandData(ESM::Land::DATA_VHGT);
                if (landData != nullptr)
                {
                    // Ensure data is loaded if necessary
                    land->loadData(ESM::Land::DATA_VHGT);
                    landData = land->getLandData(ESM::Land::DATA_VHGT);
                }
            }
            if (landData == nullptr)
            {
                // If we failed to load data, return the default height
                return static_cast<float>(ESM::Land::DEFAULT_HEIGHT);
            }
            return ESMTerrain::Storage::getHeightAt(landData->mHeights, landData->sLandSize, pos, cellSize);
        };

        api["getLandTextureAt"] = [lua = context.mLua](const osg::Vec3f& pos, sol::object cellOrName) {
            sol::variadic_results values;
            ESM::RefId worldspace;
            if (cellOrName.is<GCell>())
                worldspace = cellOrName.as<GCell>().mStore->getCell()->getWorldSpace();
            else if (cellOrName.is<std::string_view>() && !cellOrName.as<std::string_view>().empty())
                worldspace = MWBase::Environment::get()
                                 .getWorldModel()
                                 ->getCell(cellOrName.as<std::string_view>())
                                 .getCell()
                                 ->getWorldSpace();
            else
                worldspace = ESM::Cell::sDefaultWorldspaceId;

            const float cellSize = ESM::getCellSize(worldspace);

            int cellX = static_cast<int>(std::floor(pos.x() / cellSize));
            int cellY = static_cast<int>(std::floor(pos.y() / cellSize));

            auto store = MWBase::Environment::get().getESMStore();
            // We need to read land twice. Once to get the amount of texture samples per cell edge, and the second time
            // to get the actual data
            auto landStore = store->get<ESM::Land>();
            auto land = landStore.search(cellX, cellY);
            const ESM::Land::LandData* landData = nullptr;
            if (land != nullptr)
            {
                landData = land->getLandData(ESM::Land::DATA_VTEX);
                if (landData != nullptr)
                {
                    // Ensure data is loaded if necessary
                    land->loadData(ESM::Land::DATA_VTEX);
                    landData = land->getLandData(ESM::Land::DATA_VTEX);
                }
            }
            if (landData == nullptr)
            {
                // If we fail to preload land data, return, we need to be able to get *any* land to know how to correct
                // the position used to sample terrain
                return values;
            }

            const osg::Vec3f correctedPos
                = ESMTerrain::Storage::getTextureCorrectedWorldPos(pos, landData->sLandTextureSize, cellSize);
            int correctedCellX = static_cast<int>(std::floor(correctedPos.x() / cellSize));
            int correctedCellY = static_cast<int>(std::floor(correctedPos.y() / cellSize));
            auto correctedLand = landStore.search(correctedCellX, correctedCellY);
            const ESM::Land::LandData* correctedLandData = nullptr;
            if (correctedLand != nullptr)
            {
                correctedLandData = correctedLand->getLandData(ESM::Land::DATA_VTEX);
                if (correctedLandData != nullptr)
                {
                    // Ensure data is loaded if necessary
                    land->loadData(ESM::Land::DATA_VTEX);
                    correctedLandData = correctedLand->getLandData(ESM::Land::DATA_VTEX);
                }
            }
            if (correctedLandData == nullptr)
            {
                return values;
            }

            // We're passing in sLandTextureSize, NOT sLandSize like with getHeightAt
            const ESMTerrain::UniqueTextureId textureId
                = ESMTerrain::Storage::getLandTextureAt(correctedLandData->mTextures, correctedLand->getPlugin(),
                    correctedLandData->sLandTextureSize, correctedPos, cellSize);

            // Need to check for 0, 0 so that we can safely subtract 1 later, as per documentation on UniqueTextureId
            if (textureId.first != 0)
            {
                values.push_back(sol::make_object<uint16_t>(lua->sol(), textureId.first - 1));
                values.push_back(sol::make_object<int>(lua->sol(), textureId.second));

                auto textureStore = store->get<ESM::LandTexture>();
                const std::string* textureString = textureStore.search(textureId.first - 1, textureId.second);
                if (textureString)
                {
                    values.push_back(sol::make_object<std::string>(lua->sol(), *textureString));
                }
            }

            return values;
        };

        sol::table readOnlyApi = LuaUtil::makeReadOnly(api);
        return context.setTypePackage(readOnlyApi, "openmw_core");
    }
}
