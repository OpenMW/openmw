#include <components/sqlite3/db.hpp>
#include <components/sqlite3/statement.hpp>

#include <gtest/gtest.h>

namespace
{
    using namespace testing;
    using namespace Sqlite3;

    struct Query
    {
        static std::string_view text() noexcept { return "SELECT 1"; }
        static void bind(sqlite3&, sqlite3_stmt&) {}
    };

    TEST(Sqlite3StatementTest, makeStatementShouldCreateStatementWithPreparedQuery)
    {
        const auto db = makeDb(":memory:", "CREATE TABLE test ( id INTEGER )");
        const Statement statement(*db, Query{});
        EXPECT_FALSE(statement.mNeedReset);
        EXPECT_NE(statement.mHandle, nullptr);
        EXPECT_EQ(statement.mQuery.text(), "SELECT 1");
    }
}
