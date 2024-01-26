#include "luabindings.hpp"

#include <chrono>

#include <components/esm/attr.hpp>
#include <components/esm3/loadacti.hpp>
#include <components/esm3/loadalch.hpp>
#include <components/esm3/loadarmo.hpp>
#include <components/esm3/loadbook.hpp>
#include <components/esm3/loadclot.hpp>
#include <components/esm3/loadfact.hpp>
#include <components/esm3/loadmisc.hpp>
#include <components/esm3/loadskil.hpp>
#include <components/esm3/loadweap.hpp>
#include <components/lua/l10n.hpp>
#include <components/lua/luastate.hpp>
#include <components/lua/utilpackage.hpp>
#include <components/misc/strings/lower.hpp>
#include <components/version/version.hpp>

#include "../mwbase/environment.hpp"
#include "../mwbase/statemanager.hpp"
#include "../mwbase/windowmanager.hpp"
#include "../mwbase/world.hpp"
#include "../mwworld/action.hpp"
#include "../mwworld/class.hpp"
#include "../mwworld/datetimemanager.hpp"
#include "../mwworld/esmstore.hpp"
#include "../mwworld/manualref.hpp"
#include "../mwworld/store.hpp"
#include "../mwworld/worldmodel.hpp"

#include "luaevents.hpp"
#include "luamanagerimp.hpp"
#include "mwscriptbindings.hpp"
#include "objectlists.hpp"

#include "animationbindings.hpp"
#include "camerabindings.hpp"
#include "cellbindings.hpp"
#include "debugbindings.hpp"
#include "factionbindings.hpp"
#include "inputbindings.hpp"
#include "magicbindings.hpp"
#include "nearbybindings.hpp"
#include "objectbindings.hpp"
#include "postprocessingbindings.hpp"
#include "soundbindings.hpp"
#include "stats.hpp"
#include "types/types.hpp"
#include "uibindings.hpp"
#include "vfsbindings.hpp"

namespace MWLua
{
    struct CellsStore
    {
    };
}

namespace sol
{
    template <>
    struct is_automagical<MWLua::CellsStore> : std::false_type
    {
    };
}

namespace MWLua
{

    static void checkGameInitialized(LuaUtil::LuaState* lua)
    {
        if (MWBase::Environment::get().getStateManager()->getState() == MWBase::StateManager::State_NoGame)
            throw std::runtime_error(
                "This function cannot be used until the game is fully initialized.\n" + lua->debugTraceback());
    }

    static void addTimeBindings(sol::table& api, const Context& context, bool global)
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
        api["getRealFrameDuration"] = []() { return MWBase::Environment::get().getFrameDuration(); };

        if (!global)
            return;

        api["setGameTimeScale"] = [timeManager](double scale) { timeManager->setGameTimeScale(scale); };
        api["setSimulationTimeScale"] = [context, timeManager](float scale) {
            context.mLuaManager->addAction([scale, timeManager] { timeManager->setSimulationTimeScale(scale); });
        };

        api["pause"]
            = [timeManager](sol::optional<std::string_view> tag) { timeManager->pause(tag.value_or("paused")); };
        api["unpause"]
            = [timeManager](sol::optional<std::string_view> tag) { timeManager->unpause(tag.value_or("paused")); };
        api["getPausedTags"] = [timeManager](sol::this_state lua) {
            sol::table res(lua, sol::create);
            for (const std::string& tag : timeManager->getPausedTags())
                res[tag] = tag;
            return res;
        };
    }

    static sol::table initContentFilesBindings(sol::state_view& lua)
    {
        const std::vector<std::string>& contentList = MWBase::Environment::get().getWorld()->getContentFiles();
        sol::table list(lua, sol::create);
        for (size_t i = 0; i < contentList.size(); ++i)
            list[i + 1] = Misc::StringUtils::lowerCase(contentList[i]);
        sol::table res(lua, sol::create);
        res["list"] = LuaUtil::makeReadOnly(list);
        res["indexOf"] = [&contentList](std::string_view contentFile) -> sol::optional<int> {
            for (size_t i = 0; i < contentList.size(); ++i)
                if (Misc::StringUtils::ciEqual(contentList[i], contentFile))
                    return i + 1;
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

    static sol::table initCorePackage(const Context& context)
    {
        auto* lua = context.mLua;
        sol::table api(lua->sol(), sol::create);
        api["API_REVISION"] = Version::getLuaApiRevision(); // specified in CMakeLists.txt
        api["quit"] = [lua]() {
            Log(Debug::Warning) << "Quit requested by a Lua script.\n" << lua->debugTraceback();
            MWBase::Environment::get().getStateManager()->requestQuit();
        };
        api["sendGlobalEvent"] = [context](std::string eventName, const sol::object& eventData) {
            context.mLuaEvents->addGlobalEvent(
                { std::move(eventName), LuaUtil::serialize(eventData, context.mSerializer) });
        };
        api["contentFiles"] = initContentFilesBindings(lua->sol());
        api["sound"] = initCoreSoundBindings(context);
        api["vfx"] = initCoreVfxBindings(context);
        api["getFormId"] = [](std::string_view contentFile, unsigned int index) -> std::string {
            const std::vector<std::string>& contentList = MWBase::Environment::get().getWorld()->getContentFiles();
            for (size_t i = 0; i < contentList.size(); ++i)
                if (Misc::StringUtils::ciEqual(contentList[i], contentFile))
                    return ESM::RefId(ESM::FormId{ index, int(i) }).serializeText();
            throw std::runtime_error("Content file not found: " + std::string(contentFile));
        };
        addTimeBindings(api, context, false);
        api["magic"] = initCoreMagicBindings(context);
        api["stats"] = initCoreStatsBindings(context);

        initCoreFactionBindings(context);
        api["factions"] = &MWBase::Environment::get().getWorld()->getStore().get<ESM::Faction>();

        api["l10n"] = LuaUtil::initL10nLoader(lua->sol(), MWBase::Environment::get().getL10nManager());
        const MWWorld::Store<ESM::GameSetting>* gmstStore
            = &MWBase::Environment::get().getESMStore()->get<ESM::GameSetting>();
        api["getGMST"] = [lua = context.mLua, gmstStore](const std::string& setting) -> sol::object {
            const ESM::GameSetting* gmst = gmstStore->search(setting);
            if (gmst == nullptr)
                return sol::nil;
            const ESM::Variant& value = gmst->mValue;
            switch (value.getType())
            {
                case ESM::VT_Float:
                    return sol::make_object<float>(lua->sol(), value.getFloat());
                case ESM::VT_Short:
                case ESM::VT_Long:
                case ESM::VT_Int:
                    return sol::make_object<int>(lua->sol(), value.getInteger());
                case ESM::VT_String:
                    return sol::make_object<std::string>(lua->sol(), value.getString());
                case ESM::VT_Unknown:
                case ESM::VT_None:
                    break;
            }
            return sol::nil;
        };

        return LuaUtil::makeReadOnly(api);
    }

    static void addCellGetters(sol::table& api, const Context& context)
    {
        api["getCellByName"] = [](std::string_view name) {
            return GCell{ &MWBase::Environment::get().getWorldModel()->getCell(name, /*forceLoad=*/false) };
        };
        api["getExteriorCell"] = [](int x, int y, sol::object cellOrName) {
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
            return GCell{ &MWBase::Environment::get().getWorldModel()->getExterior(
                ESM::ExteriorCellLocation(x, y, worldspace), /*forceLoad=*/false) };
        };

        const MWWorld::Store<ESM::Cell>* cells3Store = &MWBase::Environment::get().getESMStore()->get<ESM::Cell>();
        const MWWorld::Store<ESM4::Cell>* cells4Store = &MWBase::Environment::get().getESMStore()->get<ESM4::Cell>();
        sol::usertype<CellsStore> cells = context.mLua->sol().new_usertype<CellsStore>("Cells");
        cells[sol::meta_function::length]
            = [cells3Store, cells4Store](const CellsStore&) { return cells3Store->getSize() + cells4Store->getSize(); };
        cells[sol::meta_function::index]
            = [cells3Store, cells4Store](const CellsStore&, size_t index) -> sol::optional<GCell> {
            if (index > cells3Store->getSize() + cells3Store->getSize() || index == 0)
                return sol::nullopt;

            index--; // Translate from Lua's 1-based indexing.
            if (index < cells3Store->getSize())
            {
                const ESM::Cell* cellRecord = cells3Store->at(index);
                return GCell{ &MWBase::Environment::get().getWorldModel()->getCell(
                    cellRecord->mId, /*forceLoad=*/false) };
            }
            else
            {
                const ESM4::Cell* cellRecord = cells4Store->at(index - cells3Store->getSize());
                return GCell{ &MWBase::Environment::get().getWorldModel()->getCell(
                    cellRecord->mId, /*forceLoad=*/false) };
            }
        };
        cells[sol::meta_function::pairs] = context.mLua->sol()["ipairsForArray"].template get<sol::function>();
        cells[sol::meta_function::ipairs] = context.mLua->sol()["ipairsForArray"].template get<sol::function>();
        api["cells"] = CellsStore{};
    }

    static sol::table initWorldPackage(const Context& context)
    {
        sol::table api(context.mLua->sol(), sol::create);
        ObjectLists* objectLists = context.mObjectLists;
        addTimeBindings(api, context, true);
        addCellGetters(api, context);
        api["mwscript"] = initMWScriptBindings(context);
        api["activeActors"] = GObjectList{ objectLists->getActorsInScene() };
        api["players"] = GObjectList{ objectLists->getPlayers() };
        api["createObject"] = [lua = context.mLua](std::string_view recordId, sol::optional<int> count) -> GObject {
            checkGameInitialized(lua);
            MWWorld::ManualRef mref(*MWBase::Environment::get().getESMStore(), ESM::RefId::deserializeText(recordId));
            const MWWorld::Ptr& ptr = mref.getPtr();
            ptr.getRefData().disable();
            MWWorld::CellStore& cell = MWBase::Environment::get().getWorldModel()->getDraftCell();
            MWWorld::Ptr newPtr = ptr.getClass().copyToCell(ptr, cell, count.value_or(1));
            return GObject(newPtr);
        };
        api["getObjectByFormId"] = [](std::string_view formIdStr) -> GObject {
            ESM::RefId refId = ESM::RefId::deserializeText(formIdStr);
            if (!refId.is<ESM::FormId>())
                throw std::runtime_error("FormId expected, got " + std::string(formIdStr) + "; use core.getFormId");
            return GObject(*refId.getIf<ESM::FormId>());
        };

        // Creates a new record in the world database.
        api["createRecord"] = sol::overload(
            [lua = context.mLua](const ESM::Activator& activator) -> const ESM::Activator* {
                checkGameInitialized(lua);
                return MWBase::Environment::get().getESMStore()->insert(activator);
            },
            [lua = context.mLua](const ESM::Armor& armor) -> const ESM::Armor* {
                checkGameInitialized(lua);
                return MWBase::Environment::get().getESMStore()->insert(armor);
            },
            [lua = context.mLua](const ESM::Clothing& clothing) -> const ESM::Clothing* {
                checkGameInitialized(lua);
                return MWBase::Environment::get().getESMStore()->insert(clothing);
            },
            [lua = context.mLua](const ESM::Book& book) -> const ESM::Book* {
                checkGameInitialized(lua);
                return MWBase::Environment::get().getESMStore()->insert(book);
            },
            [lua = context.mLua](const ESM::Miscellaneous& misc) -> const ESM::Miscellaneous* {
                checkGameInitialized(lua);
                return MWBase::Environment::get().getESMStore()->insert(misc);
            },
            [lua = context.mLua](const ESM::Potion& potion) -> const ESM::Potion* {
                checkGameInitialized(lua);
                return MWBase::Environment::get().getESMStore()->insert(potion);
            },
            [lua = context.mLua](const ESM::Weapon& weapon) -> const ESM::Weapon* {
                checkGameInitialized(lua);
                return MWBase::Environment::get().getESMStore()->insert(weapon);
            });

        api["_runStandardActivationAction"] = [context](const GObject& object, const GObject& actor) {
            if (!object.ptr().getRefData().activate())
                return;
            context.mLuaManager->addAction(
                [object, actor] {
                    const MWWorld::Ptr& objPtr = object.ptr();
                    const MWWorld::Ptr& actorPtr = actor.ptr();
                    objPtr.getClass().activate(objPtr, actorPtr)->execute(actorPtr);
                },
                "_runStandardActivationAction");
        };
        api["_runStandardUseAction"] = [context](const GObject& object, const GObject& actor, bool force) {
            context.mLuaManager->addAction(
                [object, actor, force] {
                    const MWWorld::Ptr& actorPtr = actor.ptr();
                    const MWWorld::Ptr& objectPtr = object.ptr();
                    if (actorPtr == MWBase::Environment::get().getWorld()->getPlayerPtr())
                        MWBase::Environment::get().getWindowManager()->useItem(objectPtr, force);
                    else
                    {
                        std::unique_ptr<MWWorld::Action> action = objectPtr.getClass().use(objectPtr, force);
                        action->execute(actorPtr, true);
                    }
                },
                "_runStandardUseAction");
        };

        return LuaUtil::makeReadOnly(api);
    }

    std::map<std::string, sol::object> initCommonPackages(const Context& context)
    {
        sol::state_view lua = context.mLua->sol();
        MWWorld::DateTimeManager* tm = MWBase::Environment::get().getWorld()->getTimeManager();
        return {
            { "openmw.animation", initAnimationPackage(context) },
            { "openmw.async",
                LuaUtil::getAsyncPackageInitializer(
                    lua, [tm] { return tm->getSimulationTime(); }, [tm] { return tm->getGameTime(); }) },
            { "openmw.core", initCorePackage(context) },
            { "openmw.types", initTypesPackage(context) },
            { "openmw.util", LuaUtil::initUtilPackage(lua) },
            { "openmw.vfs", initVFSPackage(context) },
        };
    }

    std::map<std::string, sol::object> initGlobalPackages(const Context& context)
    {
        initObjectBindingsForGlobalScripts(context);
        initCellBindingsForGlobalScripts(context);
        return {
            { "openmw.world", initWorldPackage(context) },
        };
    }

    std::map<std::string, sol::object> initLocalPackages(const Context& context)
    {
        initObjectBindingsForLocalScripts(context);
        initCellBindingsForLocalScripts(context);
        LocalScripts::initializeSelfPackage(context);
        return {
            { "openmw.nearby", initNearbyPackage(context) },
        };
    }

    std::map<std::string, sol::object> initPlayerPackages(const Context& context)
    {
        return {
            { "openmw.ambient", initAmbientPackage(context) },
            { "openmw.camera", initCameraPackage(context.mLua->sol()) },
            { "openmw.debug", initDebugPackage(context) },
            { "openmw.input", initInputPackage(context) },
            { "openmw.postprocessing", initPostprocessingPackage(context) },
            { "openmw.ui", initUserInterfacePackage(context) },
        };
    }

}
