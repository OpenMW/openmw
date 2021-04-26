#include "gmock/gmock.h"
#include <gtest/gtest.h>

#include <components/queries/luabindings.hpp>

namespace
{
    using namespace testing;

    TEST(LuaQueryPackageTest, basic)
    {
        sol::state lua;
        lua.open_libraries(sol::lib::base, sol::lib::string);
        Queries::registerQueryBindings(lua);
        lua["query"] = Queries::Query("test");
        lua["fieldX"] = Queries::Field({ "x" }, typeid(std::string));
        lua["fieldY"] = Queries::Field({ "y" }, typeid(int));
        lua.safe_script("t = query:where(fieldX:eq('abc') + fieldX:like('%abcd%'))");
        lua.safe_script("t = t:where(fieldY:gt(5))");
        lua.safe_script("t = t:orderBy(fieldX)");
        lua.safe_script("t = t:orderByDesc(fieldY)");
        lua.safe_script("t = t:groupBy(fieldY)");
        lua.safe_script("t = t:limit(10):offset(5)");
        EXPECT_EQ(
            lua.safe_script("return tostring(t)").get<std::string>(),
            "SELECT test WHERE ((x == \"abc\") OR (x LIKE \"%abcd%\")) AND (y > 5) ORDER BY x, y DESC GROUP BY y LIMIT 10 OFFSET 5");
    }
}

