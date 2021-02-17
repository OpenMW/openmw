#include "luabindings.hpp"

#include <components/lua/luastate.hpp>
#include <components/queries/luabindings.hpp>

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
        api["selectObjects"] = [context](const Queries::Query& query)
        {
            ObjectIdList list;
            WorldView* worldView = context.mWorldView;
            if (query.mQueryType == "activators")
                list = worldView->getActivatorsInScene();
            else if (query.mQueryType == "actors")
                list = worldView->getActorsInScene();
            else if (query.mQueryType == "containers")
                list = worldView->getContainersInScene();
            else if (query.mQueryType == "doors")
                list = worldView->getDoorsInScene();
            else if (query.mQueryType == "items")
                list = worldView->getItemsInScene();
            return GObjectList{selectObjectsFromList(query, list, context)};
            // TODO: Use sqlite to search objects that are not in the scene
            // return GObjectList{worldView->selectObjects(query, false)};
        };
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
        api["selectObjects"] = [context](const Queries::Query& query)
        {
            ObjectIdList list;
            WorldView* worldView = context.mWorldView;
            if (query.mQueryType == "activators")
                list = worldView->getActivatorsInScene();
            else if (query.mQueryType == "actors")
                list = worldView->getActorsInScene();
            else if (query.mQueryType == "containers")
                list = worldView->getContainersInScene();
            else if (query.mQueryType == "doors")
                list = worldView->getDoorsInScene();
            else if (query.mQueryType == "items")
                list = worldView->getItemsInScene();
            return LObjectList{selectObjectsFromList(query, list, context)};
            // TODO: Maybe use sqlite
            // return LObjectList{worldView->selectObjects(query, true)};
        };
        return context.mLua->makeReadOnly(api);
    }

    sol::table initQueryPackage(const Context& context)
    {
        Queries::registerQueryBindings(context.mLua->sol());
        sol::table query(context.mLua->sol(), sol::create);
        for (std::string_view t : ObjectQueryTypes::types)
            query[t] = Queries::Query(std::string(t));
        for (const QueryFieldGroup& group : getBasicQueryFieldGroups())
            query[group.mName] = initFieldGroup(context, group);
        return query;  // makeReadonly is applied by LuaState::addCommonPackage
    }

    sol::table initFieldGroup(const Context& context, const QueryFieldGroup& group)
    {
        sol::table res(context.mLua->sol(), sol::create);
        for (const Queries::Field* field : group.mFields)
        {
            sol::table subgroup = res;
            for (int i = 0; i < static_cast<int>(field->path().size()) - 1; ++i)
            {
                const std::string& name = field->path()[i];
                if (subgroup[name] == sol::nil)
                    subgroup[name] = context.mLua->makeReadOnly(context.mLua->newTable());
                subgroup = context.mLua->getMutableFromReadOnly(subgroup[name]);
            }
            subgroup[field->path().back()] = field;
        }
        return context.mLua->makeReadOnly(res);
    }

}

