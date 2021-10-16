#include <components/sqlite3/db.hpp>

#include <gtest/gtest.h>

namespace
{
    using namespace testing;
    using namespace Sqlite3;

    TEST(Sqlite3DbTest, makeDbShouldCreateInMemoryDbWithSchema)
    {
        const auto db = makeDb(":memory:", "CREATE TABLE test ( id INTEGER )");
        EXPECT_NE(db, nullptr);
    }
}
