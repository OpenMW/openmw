#include <components/sqlite3/db.hpp>
#include <components/sqlite3/request.hpp>
#include <components/sqlite3/statement.hpp>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <limits>
#include <tuple>
#include <vector>

namespace
{
    using namespace testing;
    using namespace Sqlite3;

    template <class T>
    struct InsertInt
    {
        static std::string_view text() noexcept { return "INSERT INTO ints (value) VALUES (:value)"; }

        static void bind(sqlite3& db, sqlite3_stmt& statement, T value)
        {
            bindParameter(db, statement, ":value", value);
        }
    };

    struct InsertReal
    {
        static std::string_view text() noexcept { return "INSERT INTO reals (value) VALUES (:value)"; }

        static void bind(sqlite3& db, sqlite3_stmt& statement, double value)
        {
            bindParameter(db, statement, ":value", value);
        }
    };

    struct InsertText
    {
        static std::string_view text() noexcept { return "INSERT INTO texts (value) VALUES (:value)"; }

        static void bind(sqlite3& db, sqlite3_stmt& statement, std::string_view value)
        {
            bindParameter(db, statement, ":value", value);
        }
    };

    struct InsertBlob
    {
        static std::string_view text() noexcept { return "INSERT INTO blobs (value) VALUES (:value)"; }

        static void bind(sqlite3& db, sqlite3_stmt& statement, const std::vector<std::byte>& value)
        {
            bindParameter(db, statement, ":value", value);
        }
    };

    struct GetAll
    {
        std::string mQuery;

        explicit GetAll(const std::string& table)
            : mQuery("SELECT value FROM " + table + " ORDER BY value")
        {
        }

        std::string_view text() noexcept { return mQuery; }
        static void bind(sqlite3&, sqlite3_stmt&) {}
    };

    template <class T>
    struct GetExact
    {
        std::string mQuery;

        explicit GetExact(const std::string& table)
            : mQuery("SELECT value FROM " + table + " WHERE value = :value")
        {
        }

        std::string_view text() noexcept { return mQuery; }

        static void bind(sqlite3& db, sqlite3_stmt& statement, const T& value)
        {
            bindParameter(db, statement, ":value", value);
        }
    };

    struct GetInt64
    {
        static std::string_view text() noexcept { return "SELECT value FROM ints WHERE value = :value"; }

        static void bind(sqlite3& db, sqlite3_stmt& statement, std::int64_t value)
        {
            bindParameter(db, statement, ":value", value);
        }
    };

    struct GetNull
    {
        static std::string_view text() noexcept { return "SELECT NULL"; }
        static void bind(sqlite3&, sqlite3_stmt&) {}
    };

    struct Int
    {
        int mValue = 0;

        Int() = default;

        explicit Int(int value)
            : mValue(value)
        {
        }

        Int& operator=(int value)
        {
            mValue = value;
            return *this;
        }

        friend bool operator==(const Int& l, const Int& r) { return l.mValue == r.mValue; }
    };

    constexpr const char schema[] = R"(
        CREATE TABLE ints ( value INTEGER );
        CREATE TABLE reals ( value REAL );
        CREATE TABLE texts ( value TEXT );
        CREATE TABLE blobs ( value BLOB );
    )";

    struct Sqlite3RequestTest : Test
    {
        const Db mDb = makeDb(":memory:", schema);
    };

    TEST_F(Sqlite3RequestTest, executeShouldSupportInt)
    {
        Statement insert(*mDb, InsertInt<int>{});
        EXPECT_EQ(execute(*mDb, insert, 13), 1);
        EXPECT_EQ(execute(*mDb, insert, 42), 1);
        Statement select(*mDb, GetAll("ints"));
        std::vector<std::tuple<int>> result;
        request(*mDb, select, std::back_inserter(result), std::numeric_limits<std::size_t>::max());
        EXPECT_THAT(result, ElementsAre(std::tuple(13), std::tuple(42)));
    }

    TEST_F(Sqlite3RequestTest, executeShouldSupportInt64)
    {
        Statement insert(*mDb, InsertInt<std::int64_t>{});
        const std::int64_t value = 1099511627776;
        EXPECT_EQ(execute(*mDb, insert, value), 1);
        Statement select(*mDb, GetAll("ints"));
        std::vector<std::tuple<std::int64_t>> result;
        request(*mDb, select, std::back_inserter(result), std::numeric_limits<std::size_t>::max());
        EXPECT_THAT(result, ElementsAre(std::tuple(value)));
    }

    TEST_F(Sqlite3RequestTest, executeShouldSupportReal)
    {
        Statement insert(*mDb, InsertReal{});
        EXPECT_EQ(execute(*mDb, insert, 3.14), 1);
        Statement select(*mDb, GetAll("reals"));
        std::vector<std::tuple<double>> result;
        request(*mDb, select, std::back_inserter(result), std::numeric_limits<std::size_t>::max());
        EXPECT_THAT(result, ElementsAre(std::tuple(3.14)));
    }

    TEST_F(Sqlite3RequestTest, executeShouldSupportText)
    {
        Statement insert(*mDb, InsertText{});
        const std::string text = "foo";
        EXPECT_EQ(execute(*mDb, insert, text), 1);
        Statement select(*mDb, GetAll("texts"));
        std::vector<std::tuple<std::string>> result;
        request(*mDb, select, std::back_inserter(result), std::numeric_limits<std::size_t>::max());
        EXPECT_THAT(result, ElementsAre(std::tuple(text)));
    }

    TEST_F(Sqlite3RequestTest, executeShouldSupportBlob)
    {
        Statement insert(*mDb, InsertBlob{});
        const std::vector<std::byte> blob({ std::byte(42), std::byte(13) });
        EXPECT_EQ(execute(*mDb, insert, blob), 1);
        Statement select(*mDb, GetAll("blobs"));
        std::vector<std::tuple<std::vector<std::byte>>> result;
        request(*mDb, select, std::back_inserter(result), std::numeric_limits<std::size_t>::max());
        EXPECT_THAT(result, ElementsAre(std::tuple(blob)));
    }

    TEST_F(Sqlite3RequestTest, requestShouldSupportInt)
    {
        Statement insert(*mDb, InsertInt<int>{});
        const int value = 42;
        EXPECT_EQ(execute(*mDb, insert, value), 1);
        Statement select(*mDb, GetExact<int>("ints"));
        std::vector<std::tuple<int>> result;
        request(*mDb, select, std::back_inserter(result), std::numeric_limits<std::size_t>::max(), value);
        EXPECT_THAT(result, ElementsAre(std::tuple(value)));
    }

    TEST_F(Sqlite3RequestTest, requestShouldSupportInt64)
    {
        Statement insert(*mDb, InsertInt<std::int64_t>{});
        const std::int64_t value = 1099511627776;
        EXPECT_EQ(execute(*mDb, insert, value), 1);
        Statement select(*mDb, GetExact<std::int64_t>("ints"));
        std::vector<std::tuple<std::int64_t>> result;
        request(*mDb, select, std::back_inserter(result), std::numeric_limits<std::size_t>::max(), value);
        EXPECT_THAT(result, ElementsAre(std::tuple(value)));
    }

    TEST_F(Sqlite3RequestTest, requestShouldSupportReal)
    {
        Statement insert(*mDb, InsertReal{});
        const double value = 3.14;
        EXPECT_EQ(execute(*mDb, insert, value), 1);
        Statement select(*mDb, GetExact<double>("reals"));
        std::vector<std::tuple<double>> result;
        request(*mDb, select, std::back_inserter(result), std::numeric_limits<std::size_t>::max(), value);
        EXPECT_THAT(result, ElementsAre(std::tuple(value)));
    }

    TEST_F(Sqlite3RequestTest, requestShouldSupportText)
    {
        Statement insert(*mDb, InsertText{});
        const std::string text = "foo";
        EXPECT_EQ(execute(*mDb, insert, text), 1);
        Statement select(*mDb, GetExact<std::string>("texts"));
        std::vector<std::tuple<std::string>> result;
        request(*mDb, select, std::back_inserter(result), std::numeric_limits<std::size_t>::max(), text);
        EXPECT_THAT(result, ElementsAre(std::tuple(text)));
    }

    TEST_F(Sqlite3RequestTest, requestShouldSupportBlob)
    {
        Statement insert(*mDb, InsertBlob{});
        const std::vector<std::byte> blob({ std::byte(42), std::byte(13) });
        EXPECT_EQ(execute(*mDb, insert, blob), 1);
        Statement select(*mDb, GetExact<std::vector<std::byte>>("blobs"));
        std::vector<std::tuple<std::vector<std::byte>>> result;
        request(*mDb, select, std::back_inserter(result), std::numeric_limits<std::size_t>::max(), blob);
        EXPECT_THAT(result, ElementsAre(std::tuple(blob)));
    }

    TEST_F(Sqlite3RequestTest, requestResultShouldSupportNull)
    {
        Statement select(*mDb, GetNull{});
        std::vector<std::tuple<void*>> result;
        request(*mDb, select, std::back_inserter(result), std::numeric_limits<std::size_t>::max());
        EXPECT_THAT(result, ElementsAre(std::tuple(nullptr)));
    }

    TEST_F(Sqlite3RequestTest, requestResultShouldSupportConstructibleFromInt)
    {
        Statement insert(*mDb, InsertInt<int>{});
        const int value = 42;
        EXPECT_EQ(execute(*mDb, insert, value), 1);
        Statement select(*mDb, GetExact<int>("ints"));
        std::vector<std::tuple<Int>> result;
        request(*mDb, select, std::back_inserter(result), std::numeric_limits<std::size_t>::max(), value);
        EXPECT_THAT(result, ElementsAre(std::tuple(Int(value))));
    }

    TEST_F(Sqlite3RequestTest, requestShouldLimitOutput)
    {
        Statement insert(*mDb, InsertInt<int>{});
        EXPECT_EQ(execute(*mDb, insert, 13), 1);
        EXPECT_EQ(execute(*mDb, insert, 42), 1);
        Statement select(*mDb, GetAll("ints"));
        std::vector<std::tuple<int>> result;
        request(*mDb, select, std::back_inserter(result), 1);
        EXPECT_THAT(result, ElementsAre(std::tuple(13)));
    }
}
