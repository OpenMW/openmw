#include <components/sqlite3/error.hpp>

#include <gtest/gtest.h>

#include <sqlite3.h>

namespace
{
    using namespace testing;
    using namespace Sqlite3;

    TEST(Sqlite3ErrorTest, getCodeShouldReturnExtendedCode)
    {
        const Error error(SQLITE_CONSTRAINT_PRIMARYKEY, "message");
        EXPECT_EQ(error.getCode(), SQLITE_CONSTRAINT_PRIMARYKEY);
    }

    TEST(Sqlite3ErrorTest, getPrimaryCodeShouldReturnPrimaryCode)
    {
        const Error error(SQLITE_CONSTRAINT_PRIMARYKEY, "message");
        EXPECT_EQ(error.getPrimaryCode(), SQLITE_CONSTRAINT);
    }

    TEST(Sqlite3ErrorTest, whatShouldReturnMessage)
    {
        const Error error(SQLITE_CONSTRAINT_PRIMARYKEY, "message");
        EXPECT_STREQ(error.what(), "message");
    }
}
