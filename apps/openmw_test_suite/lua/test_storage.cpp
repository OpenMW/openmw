#include <filesystem>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <components/lua/storage.hpp>

namespace
{
    using namespace testing;

    template <typename T>
    T get(sol::state& lua, std::string luaCode)
    {
        return lua.safe_script("return " + luaCode).get<T>();
    }

    TEST(LuaUtilStorageTest, Basic)
    {
        sol::state mLua;
        LuaUtil::LuaStorage::initLuaBindings(mLua);
        LuaUtil::LuaStorage storage(mLua);
        mLua["mutable"] = storage.getMutableSection("test");
        mLua["ro"] = storage.getReadOnlySection("test");

        mLua.safe_script("mutable:set('x', 5)");
        EXPECT_EQ(get<int>(mLua, "mutable:get('x')"), 5);
        EXPECT_EQ(get<int>(mLua, "ro:get('x')"), 5);
        EXPECT_FALSE(get<bool>(mLua, "mutable:wasChanged()"));
        EXPECT_TRUE(get<bool>(mLua, "ro:wasChanged()"));
        EXPECT_FALSE(get<bool>(mLua, "ro:wasChanged()"));

        EXPECT_THROW(mLua.safe_script("ro:set('y', 3)"), std::exception);

        mLua.safe_script("t1 = mutable:asTable()");
        mLua.safe_script("t2 = ro:asTable()");
        EXPECT_EQ(get<int>(mLua, "t1.x"), 5);
        EXPECT_EQ(get<int>(mLua, "t2.x"), 5);

        mLua.safe_script("mutable:reset()");
        EXPECT_TRUE(get<bool>(mLua, "ro:get('x') == nil"));

        mLua.safe_script("mutable:reset({x=4, y=7})");
        EXPECT_EQ(get<int>(mLua, "ro:get('x')"), 4);
        EXPECT_EQ(get<int>(mLua, "ro:get('y')"), 7);
        EXPECT_FALSE(get<bool>(mLua, "mutable:wasChanged()"));
        EXPECT_TRUE(get<bool>(mLua, "ro:wasChanged()"));
        EXPECT_FALSE(get<bool>(mLua, "ro:wasChanged()"));
    }

    TEST(LuaUtilStorageTest, Table)
    {
        sol::state mLua;
        LuaUtil::LuaStorage::initLuaBindings(mLua);
        LuaUtil::LuaStorage storage(mLua);
        mLua["mutable"] = storage.getMutableSection("test");
        mLua["ro"] = storage.getReadOnlySection("test");

        mLua.safe_script("mutable:set('x', { y = 'abc', z = 7 })");
        EXPECT_EQ(get<int>(mLua, "mutable:get('x').z"), 7);
        EXPECT_THROW(mLua.safe_script("mutable:get('x').z = 3"), std::exception);
        EXPECT_NO_THROW(mLua.safe_script("mutable:getCopy('x').z = 3"));
        EXPECT_EQ(get<int>(mLua, "mutable:get('x').z"), 7);
        EXPECT_EQ(get<int>(mLua, "ro:get('x').z"), 7);
        EXPECT_EQ(get<std::string>(mLua, "ro:get('x').y"), "abc");
    }

    TEST(LuaUtilStorageTest, Saving)
    {
        sol::state mLua;
        LuaUtil::LuaStorage::initLuaBindings(mLua);
        LuaUtil::LuaStorage storage(mLua);

        mLua["permanent"] = storage.getMutableSection("permanent");
        mLua["temporary"] = storage.getMutableSection("temporary");
        mLua.safe_script("temporary:removeOnExit()");
        mLua.safe_script("permanent:set('x', 1)");
        mLua.safe_script("temporary:set('y', 2)");

        std::string tmpFile = (std::filesystem::temp_directory_path() / "test_storage.bin").string();
        storage.save(tmpFile);
        EXPECT_EQ(get<int>(mLua, "permanent:get('x')"), 1);
        EXPECT_EQ(get<int>(mLua, "temporary:get('y')"), 2);

        storage.clearTemporary();
        mLua["permanent"] = storage.getMutableSection("permanent");
        mLua["temporary"] = storage.getMutableSection("temporary");
        EXPECT_EQ(get<int>(mLua, "permanent:get('x')"), 1);
        EXPECT_TRUE(get<bool>(mLua, "temporary:get('y') == nil"));

        mLua.safe_script("permanent:set('x', 3)");
        mLua.safe_script("permanent:set('z', 4)");

        LuaUtil::LuaStorage storage2(mLua);
        storage2.load(tmpFile);
        mLua["permanent"] = storage2.getMutableSection("permanent");
        mLua["temporary"] = storage2.getMutableSection("temporary");

        EXPECT_EQ(get<int>(mLua, "permanent:get('x')"), 1);
        EXPECT_TRUE(get<bool>(mLua, "permanent:get('z') == nil"));
        EXPECT_TRUE(get<bool>(mLua, "temporary:get('y') == nil"));
    }

}
