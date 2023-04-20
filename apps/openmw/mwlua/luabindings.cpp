#include "luabindings.hpp"

#include <chrono>

#include <components/esm/attr.hpp>
#include <components/esm3/activespells.hpp>
#include <components/esm3/loadalch.hpp>
#include <components/esm3/loadskil.hpp>
#include <components/lua/l10n.hpp>
#include <components/lua/luastate.hpp>

#include "../mwbase/environment.hpp"
#include "../mwbase/statemanager.hpp"
#include "../mwworld/action.hpp"
#include "../mwworld/class.hpp"
#include "../mwworld/esmstore.hpp"
#include "../mwworld/manualref.hpp"
#include "../mwworld/scene.hpp"
#include "../mwworld/store.hpp"

#include "luaevents.hpp"
#include "luamanagerimp.hpp"
#include "worldview.hpp"

#include "magicbindings.hpp"

namespace MWLua
{

    static void addTimeBindings(sol::table& api, const Context& context, bool global)
    {
        MWBase::World* world = MWBase::Environment::get().getWorld();

        api["getSimulationTime"] = [world = context.mWorldView]() { return world->getSimulationTime(); };
        api["getSimulationTimeScale"] = [world]() { return world->getSimulationTimeScale(); };
        api["getGameTime"] = [world = context.mWorldView]() { return world->getGameTime(); };
        api["getGameTimeScale"] = [world = context.mWorldView]() { return world->getGameTimeScale(); };
        api["isWorldPaused"] = [world = context.mWorldView]() { return world->isPaused(); };
        api["getRealTime"] = []() {
            return std::chrono::duration<double>(std::chrono::steady_clock::now().time_since_epoch()).count();
        };

        if (!global)
            return;

        api["setGameTimeScale"] = [world = context.mWorldView](double scale) { world->setGameTimeScale(scale); };

        api["setSimulationTimeScale"] = [context, world](float scale) {
            context.mLuaManager->addAction([scale, world] { world->setSimulationTimeScale(scale); });
        };

        // TODO: Ability to pause/resume world from Lua (needed for UI dehardcoding)
        // api["pause"] = []() {};
        // api["resume"] = []() {};
    }

    sol::table initCorePackage(const Context& context)
    {
        auto* lua = context.mLua;
        sol::table api(lua->sol(), sol::create);
        api["API_REVISION"] = 36;
        api["quit"] = [lua]() {
            Log(Debug::Warning) << "Quit requested by a Lua script.\n" << lua->debugTraceback();
            MWBase::Environment::get().getStateManager()->requestQuit();
        };
        api["sendGlobalEvent"] = [context](std::string eventName, const sol::object& eventData) {
            context.mLuaEvents->addGlobalEvent(
                { std::move(eventName), LuaUtil::serialize(eventData, context.mSerializer) });
        };
        addTimeBindings(api, context, false);
        api["magic"] = initCoreMagicBindings(context);
        api["l10n"] = LuaUtil::initL10nLoader(lua->sol(), MWBase::Environment::get().getL10nManager());
        const MWWorld::Store<ESM::GameSetting>* gmst
            = &MWBase::Environment::get().getESMStore()->get<ESM::GameSetting>();
        api["getGMST"] = [lua = context.mLua, gmst](const std::string& setting) -> sol::object {
            const ESM::Variant& value = gmst->find(setting)->mValue;
            if (value.getType() == ESM::VT_String)
                return sol::make_object<std::string>(lua->sol(), value.getString());
            else if (value.getType() == ESM::VT_Int)
                return sol::make_object<int>(lua->sol(), value.getInteger());
            else
                return sol::make_object<float>(lua->sol(), value.getFloat());
        };

        sol::table skill(context.mLua->sol(), sol::create);
        api["SKILL"] = LuaUtil::makeStrictReadOnly(skill);
        for (int id = 0; id < ESM::Skill::Length; ++id)
            skill[ESM::Skill::sSkillNames[id]] = Misc::StringUtils::lowerCase(ESM::Skill::sSkillNames[id]);

        sol::table attribute(context.mLua->sol(), sol::create);
        api["ATTRIBUTE"] = LuaUtil::makeStrictReadOnly(attribute);
        for (int id = 0; id < ESM::Attribute::Length; ++id)
            attribute[ESM::Attribute::sAttributeNames[id]]
                = Misc::StringUtils::lowerCase(ESM::Attribute::sAttributeNames[id]);

        return LuaUtil::makeReadOnly(api);
    }

    sol::table initWorldPackage(const Context& context)
    {
        sol::table api(context.mLua->sol(), sol::create);
        WorldView* worldView = context.mWorldView;
        addTimeBindings(api, context, true);
        api["getCellByName"]
            = [](std::string_view name) { return GCell{ &MWBase::Environment::get().getWorldModel()->getCell(name) }; };
        api["getExteriorCell"]
            = [](int x, int y) { return GCell{ &MWBase::Environment::get().getWorldModel()->getExterior(x, y) }; };
        api["activeActors"] = GObjectList{ worldView->getActorsInScene() };
        api["createObject"] = [](std::string_view recordId, sol::optional<int> count) -> GObject {
            // Doesn't matter which cell to use because the new object will be in disabled state.
            MWWorld::CellStore* cell = MWBase::Environment::get().getWorldScene()->getCurrentCell();

            MWWorld::ManualRef mref(*MWBase::Environment::get().getESMStore(), ESM::RefId::deserializeText(recordId));
            const MWWorld::Ptr& ptr = mref.getPtr();
            ptr.getRefData().disable();
            MWWorld::Ptr newPtr = ptr.getClass().copyToCell(ptr, *cell, count.value_or(1));
            return GObject(newPtr);
        };

        // Creates a new record in the world database.
        api["createRecord"] = sol::overload([](const ESM::Potion& potion) -> const ESM::Potion* {
            return MWBase::Environment::get().getESMStore()->insert(potion);
        }
            // TODO: add here overloads for other records
        );

        api["_runStandardActivationAction"] = [context](const GObject& object, const GObject& actor) {
            context.mLuaManager->addAction(
                [object, actor] {
                    const MWWorld::Ptr& objPtr = object.ptr();
                    const MWWorld::Ptr& actorPtr = actor.ptr();
                    objPtr.getClass().activate(objPtr, actorPtr)->execute(actorPtr);
                },
                "_runStandardActivationAction");
        };

        return LuaUtil::makeReadOnly(api);
    }

    sol::table initGlobalStoragePackage(const Context& context, LuaUtil::LuaStorage* globalStorage)
    {
        sol::table res(context.mLua->sol(), sol::create);
        res["globalSection"]
            = [globalStorage](std::string_view section) { return globalStorage->getMutableSection(section); };
        res["allGlobalSections"] = [globalStorage]() { return globalStorage->getAllSections(); };
        return LuaUtil::makeReadOnly(res);
    }

    sol::table initLocalStoragePackage(const Context& context, LuaUtil::LuaStorage* globalStorage)
    {
        sol::table res(context.mLua->sol(), sol::create);
        res["globalSection"]
            = [globalStorage](std::string_view section) { return globalStorage->getReadOnlySection(section); };
        return LuaUtil::makeReadOnly(res);
    }

    sol::table initPlayerStoragePackage(
        const Context& context, LuaUtil::LuaStorage* globalStorage, LuaUtil::LuaStorage* playerStorage)
    {
        sol::table res(context.mLua->sol(), sol::create);
        res["globalSection"]
            = [globalStorage](std::string_view section) { return globalStorage->getReadOnlySection(section); };
        res["playerSection"]
            = [playerStorage](std::string_view section) { return playerStorage->getMutableSection(section); };
        res["allPlayerSections"] = [playerStorage]() { return playerStorage->getAllSections(); };
        return LuaUtil::makeReadOnly(res);
    }

}
