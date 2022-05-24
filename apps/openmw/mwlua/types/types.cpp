#include "types.hpp"

#include <components/lua/luastate.hpp>

namespace MWLua
{
    namespace ObjectTypeName
    {
        // Names of object types in Lua.
        // These names are part of OpenMW Lua API.
        constexpr std::string_view Actor = "Actor";  // base type for NPC, Creature, Player
        constexpr std::string_view Item = "Item";  // base type for all items

        constexpr std::string_view Activator = "Activator";
        constexpr std::string_view Armor = "Armor";
        constexpr std::string_view Book = "Book";
        constexpr std::string_view Clothing = "Clothing";
        constexpr std::string_view Container = "Container";
        constexpr std::string_view Creature = "Creature";
        constexpr std::string_view Door = "Door";
        constexpr std::string_view Ingredient = "Ingredient";
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
    }

    namespace
    {
        struct LuaObjectTypeInfo
        {
            std::string_view mName;
            ESM::LuaScriptCfg::Flags mFlag = 0;
        };

        const static std::unordered_map<ESM::RecNameInts, LuaObjectTypeInfo> luaObjectTypeInfo = {
            {ESM::REC_INTERNAL_PLAYER, {ObjectTypeName::Player, ESM::LuaScriptCfg::sPlayer}},
            {ESM::REC_INTERNAL_MARKER, {ObjectTypeName::Marker}},
            {ESM::REC_ACTI, {ObjectTypeName::Activator, ESM::LuaScriptCfg::sActivator}},
            {ESM::REC_ARMO, {ObjectTypeName::Armor, ESM::LuaScriptCfg::sArmor}},
            {ESM::REC_BOOK, {ObjectTypeName::Book, ESM::LuaScriptCfg::sBook}},
            {ESM::REC_CLOT, {ObjectTypeName::Clothing, ESM::LuaScriptCfg::sClothing}},
            {ESM::REC_CONT, {ObjectTypeName::Container, ESM::LuaScriptCfg::sContainer}},
            {ESM::REC_CREA, {ObjectTypeName::Creature, ESM::LuaScriptCfg::sCreature}},
            {ESM::REC_DOOR, {ObjectTypeName::Door, ESM::LuaScriptCfg::sDoor}},
            {ESM::REC_INGR, {ObjectTypeName::Ingredient, ESM::LuaScriptCfg::sIngredient}},
            {ESM::REC_LIGH, {ObjectTypeName::Light, ESM::LuaScriptCfg::sLight}},
            {ESM::REC_MISC, {ObjectTypeName::MiscItem, ESM::LuaScriptCfg::sMiscItem}},
            {ESM::REC_NPC_, {ObjectTypeName::NPC, ESM::LuaScriptCfg::sNPC}},
            {ESM::REC_ALCH, {ObjectTypeName::Potion, ESM::LuaScriptCfg::sPotion}},
            {ESM::REC_STAT, {ObjectTypeName::Static}},
            {ESM::REC_WEAP, {ObjectTypeName::Weapon, ESM::LuaScriptCfg::sWeapon}},
            {ESM::REC_APPA, {ObjectTypeName::Apparatus}},
            {ESM::REC_LOCK, {ObjectTypeName::Lockpick}},
            {ESM::REC_PROB, {ObjectTypeName::Probe}},
            {ESM::REC_REPA, {ObjectTypeName::Repair}},
        };

    }

    std::string_view getLuaObjectTypeName(ESM::RecNameInts type, std::string_view fallback)
    {
        auto it = luaObjectTypeInfo.find(type);
        if (it != luaObjectTypeInfo.end())
            return it->second.mName;
        else
            return fallback;
    }

    std::string_view getLuaObjectTypeName(const MWWorld::Ptr& ptr)
    {
        return getLuaObjectTypeName(static_cast<ESM::RecNameInts>(ptr.getLuaType()), /*fallback=*/ptr.getTypeDescription());
    }

    ESM::LuaScriptCfg::Flags getLuaScriptFlag(const MWWorld::Ptr& ptr)
    {
        auto it = luaObjectTypeInfo.find(static_cast<ESM::RecNameInts>(ptr.getLuaType()));
        if (it != luaObjectTypeInfo.end())
            return it->second.mFlag;
        else
            return 0;
    }

    const MWWorld::Ptr& verifyType(ESM::RecNameInts recordType, const MWWorld::Ptr& ptr)
    {
        if (ptr.getType() != recordType)
        {
            std::string msg = "Requires type '";
            msg.append(getLuaObjectTypeName(recordType));
            msg.append("', but applied to ");
            msg.append(ptrToString(ptr));
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
        auto* lua = context.mLua;
        sol::table types(lua->sol(), sol::create);
        auto addType = [&](std::string_view name, std::vector<ESM::RecNameInts> recTypes,
                           std::optional<std::string_view> base = std::nullopt) -> sol::table
        {
            sol::table t(lua->sol(), sol::create);
            sol::table ro = LuaUtil::makeReadOnly(t);
            sol::table meta = ro[sol::metatable_key];
            meta[sol::meta_function::to_string] = [name]() { return name; };
            if (base)
            {
                t["baseType"] = types[*base];
                sol::table baseMeta(lua->sol(), sol::create);
                baseMeta[sol::meta_function::index] = LuaUtil::getMutableFromReadOnly(types[*base]);
                t[sol::metatable_key] = baseMeta;
            }
            t["objectIsInstance"] = [types=recTypes](const Object& o)
            {
                unsigned int type = o.ptr().getLuaType();
                for (ESM::RecNameInts t : types)
                    if (t == type)
                        return true;
                return false;
            };
            types[name] = ro;
            return t;
        };

        addActorBindings(addType(ObjectTypeName::Actor, {ESM::REC_INTERNAL_PLAYER, ESM::REC_CREA, ESM::REC_NPC_}), context);
        addType(ObjectTypeName::Item, {ESM::REC_ARMO, ESM::REC_BOOK, ESM::REC_CLOT, ESM::REC_INGR,
                                       ESM::REC_LIGH, ESM::REC_MISC, ESM::REC_ALCH, ESM::REC_WEAP,
                                       ESM::REC_APPA, ESM::REC_LOCK, ESM::REC_PROB, ESM::REC_REPA});

        addCreatureBindings(addType(ObjectTypeName::Creature, {ESM::REC_CREA}, ObjectTypeName::Actor), context);
        addNpcBindings(addType(ObjectTypeName::NPC, {ESM::REC_INTERNAL_PLAYER, ESM::REC_NPC_}, ObjectTypeName::Actor), context);
        addType(ObjectTypeName::Player, {ESM::REC_INTERNAL_PLAYER}, ObjectTypeName::NPC);

        addType(ObjectTypeName::Armor, {ESM::REC_ARMO}, ObjectTypeName::Item);
        addType(ObjectTypeName::Clothing, {ESM::REC_CLOT}, ObjectTypeName::Item);
        addType(ObjectTypeName::Ingredient, {ESM::REC_INGR}, ObjectTypeName::Item);
        addType(ObjectTypeName::Light, {ESM::REC_LIGH}, ObjectTypeName::Item);
        addType(ObjectTypeName::MiscItem, {ESM::REC_MISC}, ObjectTypeName::Item);
        addType(ObjectTypeName::Potion, {ESM::REC_ALCH}, ObjectTypeName::Item);
        addWeaponBindings(addType(ObjectTypeName::Weapon, {ESM::REC_WEAP}, ObjectTypeName::Item), context);
        addBookBindings(addType(ObjectTypeName::Book, {ESM::REC_BOOK}, ObjectTypeName::Item), context);
        addLockpickBindings(addType(ObjectTypeName::Lockpick, {ESM::REC_LOCK}, ObjectTypeName::Item), context);
        addProbeBindings(addType(ObjectTypeName::Probe, {ESM::REC_PROB}, ObjectTypeName::Item), context);
        addType(ObjectTypeName::Apparatus, {ESM::REC_APPA}, ObjectTypeName::Item);
        addType(ObjectTypeName::Repair, {ESM::REC_REPA}, ObjectTypeName::Item);

        addActivatorBindings(addType(ObjectTypeName::Activator, {ESM::REC_ACTI}), context);
        addContainerBindings(addType(ObjectTypeName::Container, {ESM::REC_CONT}), context);
        addDoorBindings(addType(ObjectTypeName::Door, {ESM::REC_DOOR}), context);
        addType(ObjectTypeName::Static, {ESM::REC_STAT});

        sol::table typeToPackage = getTypeToPackageTable(context.mLua->sol());
        sol::table packageToType = getPackageToTypeTable(context.mLua->sol());
        for (const auto& [type, v] : luaObjectTypeInfo)
        {
            sol::object t = types[v.mName];
            if (t == sol::nil)
                continue;
            typeToPackage[type] = t;
            packageToType[t] = type;
        }

        return LuaUtil::makeReadOnly(types);
    }
}
