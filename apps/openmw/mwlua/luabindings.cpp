#include "luabindings.hpp"

#include <components/lua/luastate.hpp>

#include "eventqueue.hpp"
#include "worldview.hpp"

namespace MWLua
{

    static sol::table definitionList(LuaUtil::LuaState& lua, std::initializer_list<std::string> values)
    {
        sol::table res(lua.sol(), sol::create);
        for (const std::string& v : values)
            res[v] = v;
        return lua.makeReadOnly(res);
    }

    sol::table initCorePackage(const Context& context)
    {
        sol::table api(context.mLua->sol(), sol::create);
        api["sendGlobalEvent"] = [context](std::string eventName, const sol::object& eventData)
        {
            context.mGlobalEventQueue->push_back({std::move(eventName), LuaUtil::serialize(eventData, context.mSerializer)});
        };
        api["getGameTimeInSeconds"] = [world=context.mWorldView]() { return world->getGameTimeInSeconds(); };
        api["getGameTimeInHours"] = [world=context.mWorldView]() { return world->getGameTimeInHours(); };
        api["OBJECT_TYPE"] = definitionList(*context.mLua,
        {
            "Activator", "Armor", "Book", "Clothing", "Creature", "Door", "Ingredient",
            "Light", "Miscellaneous", "NPC", "Player", "Potion", "Static", "Weapon"
        });
        return context.mLua->makeReadOnly(api);
    }

    sol::table initWorldPackage(const Context& context)
    {
        sol::table api(context.mLua->sol(), sol::create);
        WorldView* worldView = context.mWorldView;
        api["activeActors"] = GObjectList{worldView->getActorsInScene()};
        return context.mLua->makeReadOnly(api);
    }

    sol::table initNearbyPackage(const Context& context)
    {
        sol::table api(context.mLua->sol(), sol::create);
        WorldView* worldView = context.mWorldView;
        api["activators"] = LObjectList{worldView->getActivatorsInScene()};
        api["actors"] = LObjectList{worldView->getActorsInScene()};
        api["containers"] = LObjectList{worldView->getContainersInScene()};
        api["doors"] = LObjectList{worldView->getDoorsInScene()};
        api["items"] = LObjectList{worldView->getItemsInScene()};
        return context.mLua->makeReadOnly(api);
    }

}

