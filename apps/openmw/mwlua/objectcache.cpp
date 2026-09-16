#include "object.hpp"

#include <limits>

namespace MWLua
{
    namespace
    {
        // Lua number keys use packed doubles.
        static_assert(std::numeric_limits<double>::digits >= 53);

        constexpr double packId(const ObjectId& id)
        {
            return static_cast<double>(
                static_cast<int64_t>(id.mIndex) + static_cast<int64_t>(id.mContentFile) * (int64_t{ 1 } << 33));
        }

        enum CacheSlot
        {
            LocalObjects,
            GlobalObjects,
            LocalCells,
            GlobalCells,
            SlotCount
        };

        // Registry-local keys avoid cross-state references.
        const char sSlotKeys[SlotCount] = {};

        void* slotKey(CacheSlot slot)
        {
            return const_cast<char*>(&sSlotKeys[slot]);
        }

        void pushCacheTable(lua_State* state, CacheSlot slot)
        {
            lua_pushlightuserdata(state, slotKey(slot));
            lua_rawget(state, LUA_REGISTRYINDEX);
            if (lua_istable(state, -1))
                return;
            lua_pop(state, 1);

            sol::table cache = sol::table::create(state);
            cache[sol::metatable_key] = sol::table::create_with(state, "__mode", "v");
            sol::state_view(state).registry().raw_set(sol::lightuserdata_value(slotKey(slot)), cache);
            cache.push();
        }

        void pushKey(lua_State* state, double key)
        {
            lua_pushnumber(state, key);
        }

        // Cell addresses stay unique until cache clearing.
        void pushKey(lua_State* state, const MWWorld::CellStore* key)
        {
            lua_pushlightuserdata(state, const_cast<MWWorld::CellStore*>(key));
        }

        template <class T, class Key>
        int pushCached(lua_State* state, const T& value, CacheSlot slot, Key key)
        {
            pushCacheTable(state, slot);
            pushKey(state, key);
            lua_rawget(state, -2);
            if (!lua_isnil(state, -1))
            {
                lua_remove(state, -2); // drop the table, leave the cached object
                return 1;
            }
            lua_pop(state, 1); // the nil
            sol::stack::push<sol::detail::as_value_tag<T>>(state, value);
            pushKey(state, key);
            lua_pushvalue(state, -2);
            lua_rawset(state, -4); // table[key] = object
            lua_remove(state, -2); // drop the table
            return 1;
        }
    }

    int pushCachedObject(lua_State* state, const LObject& value)
    {
        return pushCached(state, value, LocalObjects, packId(value.id()));
    }

    int pushCachedObject(lua_State* state, const GObject& value)
    {
        return pushCached(state, value, GlobalObjects, packId(value.id()));
    }

    int pushCachedObject(lua_State* state, const LCell& value)
    {
        return pushCached(state, value, LocalCells, value.mStore);
    }

    int pushCachedObject(lua_State* state, const GCell& value)
    {
        return pushCached(state, value, GlobalCells, value.mStore);
    }

    // Prevent reused cell addresses from aliasing.
    void clearObjectCaches(lua_State* state)
    {
        sol::table registry = sol::state_view(state).registry();
        for (int slot = 0; slot < SlotCount; ++slot)
            registry.raw_set(sol::lightuserdata_value(slotKey(static_cast<CacheSlot>(slot))), sol::nil);
    }
}
