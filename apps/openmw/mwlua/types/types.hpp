#ifndef MWLUA_TYPES_H
#define MWLUA_TYPES_H

#include <sol/sol.hpp>

#include <components/esm/defs.hpp>
#include <components/esm/luascripts.hpp>

#include "../context.hpp"

namespace MWLua
{
    std::string_view getLuaObjectTypeName(ESM::RecNameInts type, std::string_view fallback = "Unknown");
    std::string_view getLuaObjectTypeName(const MWWorld::Ptr& ptr);
    const MWWorld::Ptr& verifyType(ESM::RecNameInts type, const MWWorld::Ptr& ptr);

    sol::table getTypeToPackageTable(lua_State* L);
    sol::table getPackageToTypeTable(lua_State* L);

    // Each script has a set of flags that controls to which objects the script should be
    // automatically attached. This function maps each object types to one of the flags. 
    ESM::LuaScriptCfg::Flags getLuaScriptFlag(const MWWorld::Ptr& ptr);

    sol::table initTypesPackage(const Context& context);

    // used in initTypesPackage
    void addActivatorBindings(sol::table activator, const Context& context);
    void addBookBindings(sol::table book, const Context& context);
    void addContainerBindings(sol::table container, const Context& context);
    void addDoorBindings(sol::table door, const Context& context);
    void addActorBindings(sol::table actor, const Context& context);
    void addWeaponBindings(sol::table weapon, const Context& context);
    void addNpcBindings(sol::table npc, const Context& context);
    void addCreatureBindings(sol::table creature, const Context& context);
}

#endif // MWLUA_TYPES_H
