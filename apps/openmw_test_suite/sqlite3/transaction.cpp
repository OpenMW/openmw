#include <components/sqlite3/db.hpp>
#include <components/sqlite3/request.hpp>
#include <components/sqlite3/statement.hpp>
#include <components/sqlite3/transaction.hpp>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <limits>
#include <tuple>
#include <vector>

namespace
{
    using namespace testing;
    using namespace Sqlite3;

    struct InsertId
    {
        static std::string_view text() noexcept { return "INSERT INTO test (id) VALUES (42)"; }
        static void bind(sqlite3&, sqlite3_stmt&) {}
    };

    struct GetIds
    {
        static std::string_view text() noexcept { return "SELECT id FROM test"; }
        static void bind(sqlite3&, sqlite3_stmt&) {}
    };

    struct Sqlite3TransactionTest : Test
    {
        const Db mDb = makeDb(":memory:", "CREATE TABLE test ( id INTEGER )");

        void insertId() const
        {
            Statement insertId(*mDb, InsertId{});
            EXPECT_EQ(execute(*mDb, insertId), 1);
        }

        std::vector<std::tuple<int>> getIds() const
        {
            Statement getIds(*mDb, GetIds{});
            std::vector<std::tuple<int>> result;
            request(*mDb, getIds, std::back_inserter(result), std::numeric_limits<std::size_t>::max());
            return result;
        }
    };

    TEST_F(Sqlite3TransactionTest, shouldRollbackOnDestruction)
    {
        {
            const Transaction transaction(*mDb);
            insertId();
        }
        EXPECT_THAT(getIds(), IsEmpty());
    }

    TEST_F(Sqlite3TransactionTest, commitShouldCommitTransaction)
    {
        {
            Transaction transaction(*mDb);
            insertId();
            transaction.commit();
        }
        EXPECT_THAT(getIds(), ElementsAre(std::tuple(42)));
    }
}
