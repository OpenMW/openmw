#include "luabindings.hpp"

#include <components/lua/luastate.hpp>

#include "eventqueue.hpp"
#include "worldview.hpp"

namespace MWLua
{

    sol::table initCorePackage(const Context& context)
    {
        sol::table api(context.mLua->sol(), sol::create);
        api["sendGlobalEvent"] = [context](std::string eventName, const sol::object& eventData)
        {
            context.mGlobalEventQueue->push_back({std::move(eventName), LuaUtil::serialize(eventData, context.mSerializer)});
        };
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
        api["actors"] = LObjectList{worldView->getActorsInScene()};
        api["items"] = LObjectList{worldView->getItemsInScene()};
        return context.mLua->makeReadOnly(api);
    }

}

