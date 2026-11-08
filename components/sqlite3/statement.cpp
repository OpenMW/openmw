#include "statement.hpp"
#include "error.hpp"

#include <sqlite3.h>

#include <format>
#include <stdexcept>
#include <string>
#include <string_view>

namespace Sqlite3
{
    void CloseSqlite3Stmt::operator()(sqlite3_stmt* handle) const noexcept
    {
        sqlite3_finalize(handle);
    }

    StatementHandle makeStatementHandle(sqlite3& db, std::string_view query)
    {
        sqlite3_stmt* stmt = nullptr;
        if (const int ec = sqlite3_prepare_v2(&db, query.data(), static_cast<int>(query.size()), &stmt, nullptr);
            ec != SQLITE_OK)
            throw Error(sqlite3_extended_errcode(&db),
                std::format("Failed to prepare statement for query \"{}\": {}", query, sqlite3_errmsg(&db)));
        return StatementHandle(stmt);
    }
}
