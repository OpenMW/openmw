#include "statement.hpp"

#include <sqlite3.h>

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
        if (const int ec = sqlite3_prepare_v2(&db, query.data(), static_cast<int>(query.size()), &stmt, nullptr); ec != SQLITE_OK)
            throw std::runtime_error("Failed to prepare statement for query \"" + std::string(query) + "\": "
                                     + std::string(sqlite3_errmsg(&db)));
        return StatementHandle(stmt);
    }
}
