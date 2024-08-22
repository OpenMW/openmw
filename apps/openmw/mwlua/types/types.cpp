#include "types.hpp"

#include <components/lua/luastate.hpp>
#include <components/misc/resourcehelpers.hpp>

namespace MWLua
{
    namespace ObjectTypeName
    {
        // Names of object types in Lua.
        // These names are part of OpenMW Lua API.
        constexpr std::string_view Actor = "Actor"; // base type for NPC, Creature, Player
        constexpr std::string_view Item = "Item"; // base type for all items
        constexpr std::string_view Lockable = "Lockable"; // base type for doors and containers

        constexpr std::string_view Activator = "Activator";
        constexpr std::string_view Armor = "Armor";
        constexpr std::string_view Book = "Book";
        constexpr std::string_view Clothing = "Clothing";
        constexpr std::string_view Container = "Container";
        constexpr std::string_view Creature = "Creature";
        constexpr std::string_view Door = "Door";
        constexpr std::string_view Ingredient = "Ingredient";
        constexpr std::string_view LevelledCreature = "LevelledCreature";
        constexpr std::string_view Light = "Light";
        constexpr std::string_view MiscItem = "Miscellaneous";
        constexpr std::string_view NPC = "NPC";
        constexpr std::string_view Player = "Player";
        constexpr std::string_view Potion = "Potion";
        constexpr std::string_view Static = "Static";
        constexpr std::string_view Weapon = "Weapon";
        constexpr std::string_view Apparatus = "Apparatus";
        constexpr std::string_view Lockpick = "Lockpick";
        constexpr std::string_view Probe = "Probe";
        constexpr std::string_view Repair = "Repair";
        constexpr std::string_view Marker = "Marker";

        constexpr std::string_view ESM4Activator = "ESM4Activator";
        constexpr std::string_view ESM4Ammunition = "ESM4Ammunition";
        constexpr std::string_view ESM4Armor = "ESM4Armor";
        constexpr std::string_view ESM4Book = "ESM4Book";
        constexpr std::string_view ESM4Clothing = "ESM4Clothing";
        constexpr std::string_view ESM4Container = "ESM4Container";
        constexpr std::string_view ESM4Door = "ESM4Door";
        constexpr std::string_view ESM4Flora = "ESM4Flora";
        constexpr std::string_view ESM4Furniture = "ESM4Furniture";
        constexpr std::string_view ESM4Ingredient = "ESM4Ingredient";
        constexpr std::string_view ESM4ItemMod = "ESM4ItemMod";
        constexpr std::string_view ESM4Light = "ESM4Light";
        constexpr std::string_view ESM4MiscItem = "ESM4Miscellaneous";
        constexpr std::string_view ESM4MovableStatic = "ESM4MovableStatic";
        constexpr std::string_view ESM4Potion = "ESM4Potion";
        constexpr std::string_view ESM4Static = "ESM4Static";
        constexpr std::string_view ESM4StaticCollection = "ESM4StaticCollection";
        constexpr std::string_view ESM4Terminal = "ESM4Terminal";
        constexpr std::string_view ESM4Tree = "ESM4Tree";
        constexpr std::string_view ESM4Weapon = "ESM4Weapon";
    }

    namespace
    {
        const static std::unordered_map<ESM::RecNameInts, std::string_view> luaObjectTypeInfo = {
            { ESM::REC_INTERNAL_PLAYER, ObjectTypeName::Player },
            { ESM::REC_INTERNAL_MARKER, ObjectTypeName::Marker },
            { ESM::REC_ACTI, ObjectTypeName::Activator },
            { ESM::REC_ARMO, ObjectTypeName::Armor },
            { ESM::REC_BOOK, ObjectTypeName::Book },
            { ESM::REC_CLOT, ObjectTypeName::Clothing },
            { ESM::REC_CONT, ObjectTypeName::Container },
            { ESM::REC_CREA, ObjectTypeName::Creature },
            { ESM::REC_DOOR, ObjectTypeName::Door },
            { ESM::REC_INGR, ObjectTypeName::Ingredient },
            { ESM::REC_LEVC, ObjectTypeName::LevelledCreature },
            { ESM::REC_LIGH, ObjectTypeName::Light },
            { ESM::REC_MISC, ObjectTypeName::MiscItem },
            { ESM::REC_NPC_, ObjectTypeName::NPC },
            { ESM::REC_ALCH, ObjectTypeName::Potion },
            { ESM::REC_STAT, ObjectTypeName::Static },
            { ESM::REC_WEAP, ObjectTypeName::Weapon },
            { ESM::REC_APPA, ObjectTypeName::Apparatus },
            { ESM::REC_LOCK, ObjectTypeName::Lockpick },
            { ESM::REC_PROB, ObjectTypeName::Probe },
            { ESM::REC_REPA, ObjectTypeName::Repair },

            { ESM::REC_ACTI4, ObjectTypeName::ESM4Activator },
            { ESM::REC_AMMO4, ObjectTypeName::ESM4Ammunition },
            { ESM::REC_ARMO4, ObjectTypeName::ESM4Armor },
            { ESM::REC_BOOK4, ObjectTypeName::ESM4Book },
            { ESM::REC_CLOT4, ObjectTypeName::ESM4Clothing },
            { ESM::REC_CONT4, ObjectTypeName::ESM4Container },
            { ESM::REC_DOOR4, ObjectTypeName::ESM4Door },
            { ESM::REC_FLOR4, ObjectTypeName::ESM4Flora },
            { ESM::REC_FURN4, ObjectTypeName::ESM4Furniture },
            { ESM::REC_INGR4, ObjectTypeName::ESM4Ingredient },
            { ESM::REC_IMOD4, ObjectTypeName::ESM4ItemMod },
            { ESM::REC_LIGH4, ObjectTypeName::ESM4Light },
            { ESM::REC_MISC4, ObjectTypeName::ESM4MiscItem },
            { ESM::REC_MSTT4, ObjectTypeName::ESM4MovableStatic },
            { ESM::REC_ALCH4, ObjectTypeName::ESM4Potion },
            { ESM::REC_STAT4, ObjectTypeName::ESM4Static },
            { ESM::REC_SCOL4, ObjectTypeName::ESM4StaticCollection },
            { ESM::REC_TERM4, ObjectTypeName::ESM4Terminal },
            { ESM::REC_TREE4, ObjectTypeName::ESM4Tree },
            { ESM::REC_WEAP4, ObjectTypeName::ESM4Weapon },
        };
    }

    unsigned int getLiveCellRefType(const MWWorld::LiveCellRefBase* ref)
    {
        if (ref == nullptr)
            throw std::runtime_error("Can't get type name from an empty object.");
        const ESM::RefId& id = ref->mRef.getRefId();
        if (id == "Player")
            return ESM::REC_INTERNAL_PLAYER;
        if (Misc::ResourceHelpers::isHiddenMarker(id))
            return ESM::REC_INTERNAL_MARKER;
        return ref->getType();
    }

    std::string_view getLuaObjectTypeName(ESM::RecNameInts type, std::string_view fallback)
    {
        auto it = luaObjectTypeInfo.find(type);
        if (it != luaObjectTypeInfo.end())
            return it->second;
        else
            return fallback;
    }

    std::string_view getLuaObjectTypeName(const MWWorld::Ptr& ptr)
    {
        return getLuaObjectTypeName(
            static_cast<ESM::RecNameInts>(getLiveCellRefType(ptr.mRef)), /*fallback=*/ptr.getTypeDescription());
    }

    const MWWorld::Ptr& verifyType(ESM::RecNameInts recordType, const MWWorld::Ptr& ptr)
    {
        if (ptr.getType() != recordType)
        {
            std::string msg = "Requires type '";
            msg.append(getLuaObjectTypeName(recordType));
            msg.append("', but applied to ");
            msg.append(ptr.toString());
            throw std::runtime_error(msg);
        }
        return ptr;
    }

    sol::table getTypeToPackageTable(lua_State* L)
    {
        constexpr std::string_view key = "typeToPackage";
        sol::state_view lua(L);
        if (lua[key] == sol::nil)
            lua[key] = sol::table(lua, sol::create);
        return lua[key];
    }

    sol::table getPackageToTypeTable(lua_State* L)
    {
        constexpr std::string_view key = "packageToType";
        sol::state_view lua(L);
        if (lua[key] == sol::nil)
            lua[key] = sol::table(lua, sol::create);
        return lua[key];
    }

    sol::table initTypesPackage(const Context& context)
    {
        auto lua = context.sol();

        if (lua["openmw_types"] != sol::nil)
            return lua["openmw_types"];

        sol::table types(lua, sol::create);
        auto addType = [&](std::string_view name, std::vector<ESM::RecNameInts> recTypes,
                           std::optional<std::string_view> base = std::nullopt) -> sol::table {
            sol::table t(lua, sol::create);
            sol::table ro = LuaUtil::makeReadOnly(t);
            sol::table meta = ro[sol::metatable_key];
            meta[sol::meta_function::to_string] = [name]() { return name; };
            if (base)
            {
                t["baseType"] = types[*base];
                sol::table baseMeta(lua, sol::create);
                baseMeta[sol::meta_function::index] = LuaUtil::getMutableFromReadOnly(types[*base]);
                t[sol::metatable_key] = baseMeta;
            }
            t["objectIsInstance"] = [types = recTypes](const Object& o) {
                unsigned int type = getLiveCellRefType(o.ptr().mRef);
                for (ESM::RecNameInts t : types)
                    if (t == type)
                        return true;
                return false;
            };
            types[name] = ro;
            return t;
        };

        addActorBindings(
            addType(ObjectTypeName::Actor, { ESM::REC_INTERNAL_PLAYER, ESM::REC_CREA, ESM::REC_NPC_ }), context);
        addItemBindings(
            addType(ObjectTypeName::Item,
                { ESM::REC_ARMO, ESM::REC_BOOK, ESM::REC_CLOT, ESM::REC_INGR, ESM::REC_LIGH, ESM::REC_MISC,
                    ESM::REC_ALCH, ESM::REC_WEAP, ESM::REC_APPA, ESM::REC_LOCK, ESM::REC_PROB, ESM::REC_REPA }),
            context);
        addLockableBindings(
            addType(ObjectTypeName::Lockable, { ESM::REC_CONT, ESM::REC_DOOR, ESM::REC_CONT4, ESM::REC_DOOR4 }));

        addCreatureBindings(addType(ObjectTypeName::Creature, { ESM::REC_CREA }, ObjectTypeName::Actor), context);
        addNpcBindings(
            addType(ObjectTypeName::NPC, { ESM::REC_INTERNAL_PLAYER, ESM::REC_NPC_ }, ObjectTypeName::Actor), context);
        addPlayerBindings(addType(ObjectTypeName::Player, { ESM::REC_INTERNAL_PLAYER }, ObjectTypeName::NPC), context);

        addLevelledCreatureBindings(addType(ObjectTypeName::LevelledCreature, { ESM::REC_LEVC }), context);

        addArmorBindings(addType(ObjectTypeName::Armor, { ESM::REC_ARMO }, ObjectTypeName::Item), context);
        addClothingBindings(addType(ObjectTypeName::Clothing, { ESM::REC_CLOT }, ObjectTypeName::Item), context);
        addIngredientBindings(addType(ObjectTypeName::Ingredient, { ESM::REC_INGR }, ObjectTypeName::Item), context);
        addLightBindings(addType(ObjectTypeName::Light, { ESM::REC_LIGH }, ObjectTypeName::Item), context);
        addMiscellaneousBindings(addType(ObjectTypeName::MiscItem, { ESM::REC_MISC }, ObjectTypeName::Item), context);
        addPotionBindings(addType(ObjectTypeName::Potion, { ESM::REC_ALCH }, ObjectTypeName::Item), context);
        addWeaponBindings(addType(ObjectTypeName::Weapon, { ESM::REC_WEAP }, ObjectTypeName::Item), context);
        addBookBindings(addType(ObjectTypeName::Book, { ESM::REC_BOOK }, ObjectTypeName::Item), context);
        addLockpickBindings(addType(ObjectTypeName::Lockpick, { ESM::REC_LOCK }, ObjectTypeName::Item), context);
        addProbeBindings(addType(ObjectTypeName::Probe, { ESM::REC_PROB }, ObjectTypeName::Item), context);
        addApparatusBindings(addType(ObjectTypeName::Apparatus, { ESM::REC_APPA }, ObjectTypeName::Item), context);
        addRepairBindings(addType(ObjectTypeName::Repair, { ESM::REC_REPA }, ObjectTypeName::Item), context);

        addActivatorBindings(addType(ObjectTypeName::Activator, { ESM::REC_ACTI }), context);
        addContainerBindings(addType(ObjectTypeName::Container, { ESM::REC_CONT }, ObjectTypeName::Lockable), context);
        addDoorBindings(addType(ObjectTypeName::Door, { ESM::REC_DOOR }, ObjectTypeName::Lockable), context);
        addStaticBindings(addType(ObjectTypeName::Static, { ESM::REC_STAT }), context);

        addType(ObjectTypeName::ESM4Activator, { ESM::REC_ACTI4 });
        addType(ObjectTypeName::ESM4Ammunition, { ESM::REC_AMMO4 });
        addType(ObjectTypeName::ESM4Armor, { ESM::REC_ARMO4 });
        addType(ObjectTypeName::ESM4Book, { ESM::REC_BOOK4 });
        addType(ObjectTypeName::ESM4Clothing, { ESM::REC_CLOT4 });
        addType(ObjectTypeName::ESM4Container, { ESM::REC_CONT4 });
        addESM4DoorBindings(addType(ObjectTypeName::ESM4Door, { ESM::REC_DOOR4 }, ObjectTypeName::Lockable), context);
        addType(ObjectTypeName::ESM4Flora, { ESM::REC_FLOR4 });
        addType(ObjectTypeName::ESM4Furniture, { ESM::REC_FURN4 });
        addType(ObjectTypeName::ESM4Ingredient, { ESM::REC_INGR4 });
        addType(ObjectTypeName::ESM4ItemMod, { ESM::REC_IMOD4 });
        addType(ObjectTypeName::ESM4Light, { ESM::REC_LIGH4 });
        addType(ObjectTypeName::ESM4MiscItem, { ESM::REC_MISC4 });
        addType(ObjectTypeName::ESM4MovableStatic, { ESM::REC_MSTT4 });
        addType(ObjectTypeName::ESM4Potion, { ESM::REC_ALCH4 });
        addType(ObjectTypeName::ESM4Static, { ESM::REC_STAT4 });
        addType(ObjectTypeName::ESM4StaticCollection, { ESM::REC_SCOL4 });
        addESM4TerminalBindings(addType(ObjectTypeName::ESM4Terminal, { ESM::REC_TERM4 }), context);
        addType(ObjectTypeName::ESM4Tree, { ESM::REC_TREE4 });
        addType(ObjectTypeName::ESM4Weapon, { ESM::REC_WEAP4 });

        sol::table typeToPackage = getTypeToPackageTable(lua);
        sol::table packageToType = getPackageToTypeTable(lua);
        for (const auto& [type, name] : luaObjectTypeInfo)
        {
            sol::object t = types[name];
            if (t == sol::nil)
                continue;
            typeToPackage[type] = t;
            packageToType[t] = type;
        }

        lua["openmw_types"] = LuaUtil::makeReadOnly(types);
        return lua["openmw_types"];
    }
}
