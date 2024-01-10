#ifndef MWLUA_TYPES_H
#define MWLUA_TYPES_H

#include <sol/sol.hpp>

#include <components/esm/defs.hpp>
#include <components/lua/luastate.hpp>

#include "apps/openmw/mwbase/environment.hpp"
#include "apps/openmw/mwworld/esmstore.hpp"
#include "apps/openmw/mwworld/store.hpp"

#include "../context.hpp"
#include "../object.hpp"

namespace sol
{
    // Ensure sol does not try to create the automatic Container or usertype bindings for Store.
    // They include write operations and we want the store to be read-only.
    template <typename T>
    struct is_automagical<typename MWWorld::Store<T>> : std::false_type
    {
    };
}

namespace MWLua
{
    // `getLiveCellRefType()` is not exactly what we usually mean by "type" because some refids have special meaning.
    // This function handles these special refids (and by this adds some performance overhead).
    // We use this "fixed" type in Lua because we don't want to expose the weirdness of Morrowind internals to our API.
    // TODO: Implement https://gitlab.com/OpenMW/openmw/-/issues/6617 and make `MWWorld::PtrBase::getType` work the
    // same as `getLiveCellRefType`.
    unsigned int getLiveCellRefType(const MWWorld::LiveCellRefBase* ref);

    std::string_view getLuaObjectTypeName(ESM::RecNameInts type, std::string_view fallback = "Unknown");
    std::string_view getLuaObjectTypeName(const MWWorld::Ptr& ptr);
    const MWWorld::Ptr& verifyType(ESM::RecNameInts type, const MWWorld::Ptr& ptr);

    sol::table getTypeToPackageTable(lua_State* L);
    sol::table getPackageToTypeTable(lua_State* L);

    sol::table initTypesPackage(const Context& context);

    // used in initTypesPackage
    void addActivatorBindings(sol::table activator, const Context& context);
    void addBookBindings(sol::table book, const Context& context);
    void addContainerBindings(sol::table container, const Context& context);
    void addDoorBindings(sol::table door, const Context& context);
    void addItemBindings(sol::table item, const Context& context);
    void addActorBindings(sol::table actor, const Context& context);
    void addWeaponBindings(sol::table weapon, const Context& context);
    void addNpcBindings(sol::table npc, const Context& context);
    void addPlayerBindings(sol::table player, const Context& context);
    void addCreatureBindings(sol::table creature, const Context& context);
    void addLockpickBindings(sol::table lockpick, const Context& context);
    void addProbeBindings(sol::table probe, const Context& context);
    void addApparatusBindings(sol::table apparatus, const Context& context);
    void addRepairBindings(sol::table repair, const Context& context);
    void addMiscellaneousBindings(sol::table miscellaneous, const Context& context);
    void addPotionBindings(sol::table potion, const Context& context);
    void addIngredientBindings(sol::table Ingredient, const Context& context);
    void addArmorBindings(sol::table armor, const Context& context);
    void addLockableBindings(sol::table lockable);
    void addClothingBindings(sol::table clothing, const Context& context);
    void addStaticBindings(sol::table stat, const Context& context);
    void addLightBindings(sol::table light, const Context& context);
    void addLevelledCreatureBindings(sol::table list, const Context& context);

    void addESM4DoorBindings(sol::table door, const Context& context);
    void addESM4TerminalBindings(sol::table term, const Context& context);

    template <class T>
    void addRecordFunctionBinding(
        sol::table& table, const Context& context, const std::string& recordName = std::string(T::getRecordType()))
    {
        const MWWorld::Store<T>& store = MWBase::Environment::get().getESMStore()->get<T>();

        table["record"] = sol::overload([](const Object& obj) -> const T* { return obj.ptr().get<T>()->mBase; },
            [&store](std::string_view id) -> const T* { return store.search(ESM::RefId::deserializeText(id)); });

        // Define a custom user type for the store.
        // Provide the interface of a read-only array.
        using StoreT = MWWorld::Store<T>;
        sol::state_view& lua = context.mLua->sol();
        sol::usertype<StoreT> storeT = lua.new_usertype<StoreT>(recordName + "WorldStore");
        storeT[sol::meta_function::to_string] = [recordName](const StoreT& store) {
            return "{" + std::to_string(store.getSize()) + " " + recordName + " records}";
        };
        storeT[sol::meta_function::length] = [](const StoreT& store) { return store.getSize(); };
        storeT[sol::meta_function::index] = [](const StoreT& store, size_t index) -> const T* {
            if (index == 0 || index > store.getSize())
                return nullptr;
            return store.at(index - 1); // Translate from Lua's 1-based indexing.
        };
        storeT[sol::meta_function::pairs] = lua["ipairsForArray"].template get<sol::function>();
        storeT[sol::meta_function::ipairs] = lua["ipairsForArray"].template get<sol::function>();

        // Provide access to the store.
        table["records"] = &store;
    }
}

#endif // MWLUA_TYPES_H
