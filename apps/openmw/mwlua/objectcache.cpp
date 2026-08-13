#include "object.hpp"

namespace MWLua
{
    namespace
    {
        // Index in the low 32 bits, content file from bit 33, so the pair is exact as a double.
        // Generated objects have a negative content file and so land on negative keys.
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

        // Registry keys. An address rather than a cached luaL_ref, so a second Lua state gets
        // its own tables instead of reusing a reference number that means something else there.
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
            lua_pop(state, 1); // nil: this state has not pushed this type yet
            lua_newtable(state); // the cache
            lua_newtable(state); // its metatable
            lua_pushstring(state, "v");
            lua_setfield(state, -2, "__mode"); // weak values: unreferenced objects still collect
            lua_setmetatable(state, -2);
            lua_pushlightuserdata(state, slotKey(slot));
            lua_pushvalue(state, -2);
            lua_rawset(state, LUA_REGISTRYINDEX);
        }

        void pushKey(lua_State* state, double key)
        {
            lua_pushnumber(state, key);
        }

        // Cells have no packable id, so they key on the store address as light userdata.
        // Safe only because clearObjectCaches runs before an address can be reused.
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

    // The Lua state outlives the world, so stale entries would still be here when the allocator
    // hands a new cell the address of a dead one.
    void clearObjectCaches(lua_State* state)
    {
        for (int slot = 0; slot < SlotCount; ++slot)
        {
            lua_pushlightuserdata(state, slotKey(static_cast<CacheSlot>(slot)));
            lua_pushnil(state);
            lua_rawset(state, LUA_REGISTRYINDEX);
        }
    }
}
