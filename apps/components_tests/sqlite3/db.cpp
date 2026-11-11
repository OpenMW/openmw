#include <components/files/conversion.hpp>
#include <components/sqlite3/db.hpp>
#include <components/sqlite3/error.hpp>
#include <components/testing/util.hpp>

#include <gtest/gtest.h>

#include <sqlite3.h>

#include <filesystem>
#include <fstream>
#include <string>

namespace
{
    using namespace testing;
    using namespace Sqlite3;

    TEST(Sqlite3DbTest, makeDbShouldCreateInMemoryDbWithSchema)
    {
        const auto db = makeDb(":memory:", "CREATE TABLE test ( id INTEGER )");
        EXPECT_NE(db, nullptr);
    }

    TEST(Sqlite3DbTest, makeDbShouldThrowErrorWithNotADbCodeForNonDatabaseFile)
    {
        const std::filesystem::path path = TestingOpenMW::outputFilePath("Sqlite3DbTest.notadb");
        std::ofstream(path, std::ios::binary) << std::string(4096, 'x');

        try
        {
            makeDb(Files::pathToUnicodeString(path), "CREATE TABLE test ( id INTEGER )");
            FAIL() << "Sqlite3::Error is not thrown";
        }
        catch (const Error& e)
        {
            EXPECT_EQ(e.getPrimaryCode(), SQLITE_NOTADB);
        }
    }
}
