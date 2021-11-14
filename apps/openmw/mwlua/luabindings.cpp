#include "luabindings.hpp"

#include <components/lua/luastate.hpp>
#include <components/queries/luabindings.hpp>

#include "../mwbase/environment.hpp"
#include "../mwbase/statemanager.hpp"
#include "../mwworld/inventorystore.hpp"

#include "eventqueue.hpp"
#include "worldview.hpp"

namespace MWLua
{

    static sol::table definitionList(LuaUtil::LuaState& lua, std::initializer_list<std::string> values)
    {
        sol::table res(lua.sol(), sol::create);
        for (const std::string& v : values)
            res[v] = v;
        return LuaUtil::makeReadOnly(res);
    }

    sol::table initCorePackage(const Context& context)
    {
        auto* lua = context.mLua;
        sol::table api(lua->sol(), sol::create);
        api["API_REVISION"] = 9;
        api["quit"] = [lua]()
        {
            Log(Debug::Warning) << "Quit requested by a Lua script.\n" << lua->debugTraceback();
            MWBase::Environment::get().getStateManager()->requestQuit();
        };
        api["sendGlobalEvent"] = [context](std::string eventName, const sol::object& eventData)
        {
            context.mGlobalEventQueue->push_back({std::move(eventName), LuaUtil::serialize(eventData, context.mSerializer)});
        };
        api["getGameTimeInSeconds"] = [world=context.mWorldView]() { return world->getGameTimeInSeconds(); };
        api["getGameTimeInHours"] = [world=context.mWorldView]() { return world->getGameTimeInHours(); };
        api["OBJECT_TYPE"] = definitionList(*lua,
        {
            "Activator", "Armor", "Book", "Clothing", "Creature", "Door", "Ingredient",
            "Light", "Miscellaneous", "NPC", "Player", "Potion", "Static", "Weapon"
        });
        api["EQUIPMENT_SLOT"] = LuaUtil::makeReadOnly(context.mLua->tableFromPairs<std::string_view, int>({
            {"Helmet", MWWorld::InventoryStore::Slot_Helmet},
            {"Cuirass", MWWorld::InventoryStore::Slot_Cuirass},
            {"Greaves", MWWorld::InventoryStore::Slot_Greaves},
            {"LeftPauldron", MWWorld::InventoryStore::Slot_LeftPauldron},
            {"RightPauldron", MWWorld::InventoryStore::Slot_RightPauldron},
            {"LeftGauntlet", MWWorld::InventoryStore::Slot_LeftGauntlet},
            {"RightGauntlet", MWWorld::InventoryStore::Slot_RightGauntlet},
            {"Boots", MWWorld::InventoryStore::Slot_Boots},
            {"Shirt", MWWorld::InventoryStore::Slot_Shirt},
            {"Pants", MWWorld::InventoryStore::Slot_Pants},
            {"Skirt", MWWorld::InventoryStore::Slot_Skirt},
            {"Robe", MWWorld::InventoryStore::Slot_Robe},
            {"LeftRing", MWWorld::InventoryStore::Slot_LeftRing},
            {"RightRing", MWWorld::InventoryStore::Slot_RightRing},
            {"Amulet", MWWorld::InventoryStore::Slot_Amulet},
            {"Belt", MWWorld::InventoryStore::Slot_Belt},
            {"CarriedRight", MWWorld::InventoryStore::Slot_CarriedRight},
            {"CarriedLeft", MWWorld::InventoryStore::Slot_CarriedLeft},
            {"Ammunition", MWWorld::InventoryStore::Slot_Ammunition}
        }));
        return LuaUtil::makeReadOnly(api);
    }

    sol::table initWorldPackage(const Context& context)
    {
        sol::table api(context.mLua->sol(), sol::create);
        WorldView* worldView = context.mWorldView;
        api["getCellByName"] = [worldView=context.mWorldView](const std::string& name) -> sol::optional<GCell>
        {
            MWWorld::CellStore* cell = worldView->findNamedCell(name);
            if (cell)
                return GCell{cell};
            else
                return sol::nullopt;
        };
        api["getExteriorCell"] = [worldView=context.mWorldView](int x, int y) -> sol::optional<GCell>
        {
            MWWorld::CellStore* cell = worldView->findExteriorCell(x, y);
            if (cell)
                return GCell{cell};
            else
                return sol::nullopt;
        };
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
        // TODO: add world.placeNewObject(recordId, cell, pos, [rot])
        return LuaUtil::makeReadOnly(api);
    }

    sol::table initQueryPackage(const Context& context)
    {
        Queries::registerQueryBindings(context.mLua->sol());
        sol::table query(context.mLua->sol(), sol::create);
        for (std::string_view t : ObjectQueryTypes::types)
            query[t] = Queries::Query(std::string(t));
        for (const QueryFieldGroup& group : getBasicQueryFieldGroups())
            query[group.mName] = initFieldGroup(context, group);
        return query;  // makeReadOnly is applied by LuaState::addCommonPackage
    }

    sol::table initFieldGroup(const Context& context, const QueryFieldGroup& group)
    {
        sol::table res(context.mLua->sol(), sol::create);
        for (const Queries::Field* field : group.mFields)
        {
            sol::table subgroup = res;
            if (field->path().empty())
                throw std::logic_error("Empty path in Queries::Field");
            for (size_t i = 0; i < field->path().size() - 1; ++i)
            {
                const std::string& name = field->path()[i];
                if (subgroup[name] == sol::nil)
                    subgroup[name] = LuaUtil::makeReadOnly(context.mLua->newTable());
                subgroup = LuaUtil::getMutableFromReadOnly(subgroup[name]);
            }
            subgroup[field->path().back()] = field;
        }
        return LuaUtil::makeReadOnly(res);
    }

}

