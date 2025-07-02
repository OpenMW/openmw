#include "corebindings.hpp"

#include <chrono>
#include <stdexcept>

#include <components/debug/debuglog.hpp>
#include <components/esm3/loadfact.hpp>
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

#include "coremwscriptbindings.hpp"
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

        api["mwscripts"]
            = context.cachePackage("openmw_core_mwscripts", [context]() { return initCoreMwScriptBindings(context); });

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

        sol::table readOnlyApi = LuaUtil::makeReadOnly(api);
        return context.setTypePackage(readOnlyApi, "openmw_core");
    }
}
