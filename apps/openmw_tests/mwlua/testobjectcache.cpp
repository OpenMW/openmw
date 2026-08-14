#include "apps/openmw/mwlua/object.hpp"

#include <cstddef>

#include <gtest/gtest.h>

#include <components/lua/luastate.hpp>

namespace MWLua
{
    namespace
    {
        // Cache keys never dereference cells.
        MWWorld::CellStore* asCell(std::max_align_t& storage)
        {
            return reinterpret_cast<MWWorld::CellStore*>(&storage);
        }

        template <class T>
        sol::object push(sol::state_view& lua, const T& value)
        {
            return sol::make_object(lua, value);
        }

        TEST(MWLuaObjectCacheTest, shouldPushOneUserdataForRepeatedLocalObjects)
        {
            LuaUtil::LuaState state{ nullptr, nullptr };
            sol::state_view lua = state.unsafeState();
            const ObjectId id{ 42, 0 };

            const sol::object first = push(lua, LObject(id));
            const sol::object second = push(lua, LObject(id));

            EXPECT_EQ(first.pointer(), second.pointer());
        }

        TEST(MWLuaObjectCacheTest, shouldPushOneUserdataForRepeatedGlobalObjects)
        {
            LuaUtil::LuaState state{ nullptr, nullptr };
            sol::state_view lua = state.unsafeState();
            const ObjectId id{ 42, 0 };

            const sol::object first = push(lua, GObject(id));
            const sol::object second = push(lua, GObject(id));

            EXPECT_EQ(first.pointer(), second.pointer());
        }

        TEST(MWLuaObjectCacheTest, shouldPushDifferentUserdataForDifferentObjectIds)
        {
            LuaUtil::LuaState state{ nullptr, nullptr };
            sol::state_view lua = state.unsafeState();

            const sol::object first = push(lua, LObject(ObjectId{ 42, 0 }));
            const sol::object second = push(lua, LObject(ObjectId{ 43, 0 }));

            EXPECT_NE(first.pointer(), second.pointer());
        }

        TEST(MWLuaObjectCacheTest, shouldPushDifferentUserdataForIdsDifferingOnlyInContentFile)
        {
            LuaUtil::LuaState state{ nullptr, nullptr };
            sol::state_view lua = state.unsafeState();

            const sol::object first = push(lua, LObject(ObjectId{ 42, -1 }));
            const sol::object second = push(lua, LObject(ObjectId{ 42, 0 }));

            EXPECT_NE(first.pointer(), second.pointer());
        }

        TEST(MWLuaObjectCacheTest, shouldPushDifferentUserdataForALocalAndAGlobalObjectWithOneId)
        {
            LuaUtil::LuaState state{ nullptr, nullptr };
            sol::state_view lua = state.unsafeState();
            const ObjectId id{ 42, 0 };

            const sol::object local = push(lua, LObject(id));
            const sol::object global = push(lua, GObject(id));

            EXPECT_NE(local.pointer(), global.pointer());
        }

        TEST(MWLuaObjectCacheTest, shouldPushOneUserdataForRepeatedLocalCells)
        {
            LuaUtil::LuaState state{ nullptr, nullptr };
            sol::state_view lua = state.unsafeState();
            std::max_align_t storage;

            const sol::object first = push(lua, LCell{ asCell(storage) });
            const sol::object second = push(lua, LCell{ asCell(storage) });

            EXPECT_EQ(first.pointer(), second.pointer());
        }

        TEST(MWLuaObjectCacheTest, shouldPushOneUserdataForRepeatedGlobalCells)
        {
            LuaUtil::LuaState state{ nullptr, nullptr };
            sol::state_view lua = state.unsafeState();
            std::max_align_t storage;

            const sol::object first = push(lua, GCell{ asCell(storage) });
            const sol::object second = push(lua, GCell{ asCell(storage) });

            EXPECT_EQ(first.pointer(), second.pointer());
        }

        TEST(MWLuaObjectCacheTest, shouldPushDifferentUserdataForDifferentCells)
        {
            LuaUtil::LuaState state{ nullptr, nullptr };
            sol::state_view lua = state.unsafeState();
            std::max_align_t storage;
            std::max_align_t otherStorage;

            const sol::object first = push(lua, LCell{ asCell(storage) });
            const sol::object second = push(lua, LCell{ asCell(otherStorage) });

            EXPECT_NE(first.pointer(), second.pointer());
        }

        TEST(MWLuaObjectCacheTest, shouldPushDifferentUserdataForALocalAndAGlobalCellWithOneStore)
        {
            LuaUtil::LuaState state{ nullptr, nullptr };
            sol::state_view lua = state.unsafeState();
            std::max_align_t storage;

            const sol::object local = push(lua, LCell{ asCell(storage) });
            const sol::object global = push(lua, GCell{ asCell(storage) });

            EXPECT_NE(local.pointer(), global.pointer());
        }

        TEST(MWLuaObjectCacheTest, shouldPushANewUserdataAfterTheCacheIsCleared)
        {
            LuaUtil::LuaState state{ nullptr, nullptr };
            sol::state_view lua = state.unsafeState();
            std::max_align_t storage;

            const sol::object before = push(lua, LCell{ asCell(storage) });
            clearObjectCaches(lua.lua_state());
            const sol::object after = push(lua, LCell{ asCell(storage) });

            EXPECT_NE(before.pointer(), after.pointer());
        }

        TEST(MWLuaObjectCacheTest, shouldNotClearTheCacheOfAnotherLuaState)
        {
            LuaUtil::LuaState clearedState{ nullptr, nullptr };
            LuaUtil::LuaState keptState{ nullptr, nullptr };
            sol::state_view cleared = clearedState.unsafeState();
            sol::state_view kept = keptState.unsafeState();
            const ObjectId id{ 42, 0 };

            const sol::object clearedBefore = push(cleared, LObject(id));
            const sol::object keptBefore = push(kept, LObject(id));
            clearObjectCaches(cleared.lua_state());
            const sol::object clearedAfter = push(cleared, LObject(id));
            const sol::object keptAfter = push(kept, LObject(id));

            ASSERT_NE(clearedBefore.pointer(), clearedAfter.pointer());
            EXPECT_EQ(keptBefore.pointer(), keptAfter.pointer());
        }

        // Weak probes do not retain userdata.
        TEST(MWLuaObjectCacheTest, shouldNotKeepAnUnreferencedObjectAlive)
        {
            LuaUtil::LuaState state{ nullptr, nullptr };
            sol::state_view lua = state.unsafeState();
            sol::table probe = lua.create_table();
            probe[sol::metatable_key] = lua.create_table_with("__mode", "v");

            probe[1] = push(lua, LObject(ObjectId{ 42, 0 }));
            lua.collect_garbage();
            lua.collect_garbage();

            EXPECT_TRUE(probe.raw_get<sol::object>(1) == sol::nil);
        }
    }
}
