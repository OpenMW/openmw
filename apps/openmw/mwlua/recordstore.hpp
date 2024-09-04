#ifndef MWLUA_RECORDSTORE_H
#define MWLUA_RECORDSTORE_H

#include <sol/sol.hpp>

#include <components/esm/defs.hpp>
#include <components/lua/luastate.hpp>
#include <components/lua/util.hpp>

#include "apps/openmw/mwbase/environment.hpp"
#include "apps/openmw/mwbase/world.hpp"
#include "apps/openmw/mwworld/esmstore.hpp"
#include "apps/openmw/mwworld/store.hpp"

#include "context.hpp"
#include "object.hpp"

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
        sol::state_view lua = context.sol();
        sol::usertype<StoreT> storeT = lua.new_usertype<StoreT>(recordName + "WorldStore");
        storeT[sol::meta_function::to_string] = [recordName](const StoreT& store) {
            return "{" + std::to_string(store.getSize()) + " " + recordName + " records}";
        };
        storeT[sol::meta_function::length] = [](const StoreT& store) { return store.getSize(); };
        storeT[sol::meta_function::index] = sol::overload(
            [](const StoreT& store, size_t index) -> const T* {
                if (index == 0 || index > store.getSize())
                    return nullptr;
                return store.at(LuaUtil::fromLuaIndex(index));
            },
            [](const StoreT& store, std::string_view id) -> const T* {
                return store.search(ESM::RefId::deserializeText(id));
            });
        storeT[sol::meta_function::ipairs] = lua["ipairsForArray"].template get<sol::function>();
        storeT[sol::meta_function::pairs] = lua["ipairsForArray"].template get<sol::function>();

        // Provide access to the store.
        table["records"] = &store;
    }
}
#endif // MWLUA_RECORDSTORE_H
